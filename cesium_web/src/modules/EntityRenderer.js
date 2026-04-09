/**
 * EntityRenderer.js — Renders entities on Cesium map from Qt authoritative state
 *
 * Evolved from TrackVisualizer.js. This module:
 * - Creates/updates/removes aircraft entities based on state from Qt
 * - Renders waypoints, routes (polylines), labels, trails
 * - Highlights selected entity
 * - Does NOT hold any authoritative state — pure render slave
 *
 * ORIENTATION SYSTEM:
 * - Heading: degrees [0,360) true heading, 0=North, clockwise
 * - Pitch: degrees, positive = nose up (flight path angle)
 * - Roll: degrees, positive = right wing down (bank angle)
 * - CesiumJS HeadingPitchRoll uses same conventions (radians)
 *
 * INTERPOLATION:
 * - Qt sends state at 20Hz, CesiumJS renders at 60fps
 * - Dead-reckoning interpolation fills the gaps for smooth animation
 * - Uses great-circle destination formula for position extrapolation
 */

import {
  Cartesian3,
  Cartesian2,
  Color,
  LabelStyle,
  VerticalOrigin,
  HeightReference,
  HeadingPitchRoll,
  Transforms,
  Math as CesiumMath,
  ConstantPositionProperty,
  Quaternion,
  CallbackProperty,
} from "cesium";

// Earth radius for dead-reckoning calculations
const EARTH_RADIUS = 6371000.0;
const DEG_TO_RAD = Math.PI / 180.0;
const RAD_TO_DEG = 180.0 / Math.PI;

export class EntityRenderer {
  constructor(viewer) {
    this.viewer = viewer;
    this.aircraftEntities = new Map(); // id → { entity, label, trail }
    this.routeEntities = new Map(); // routeId → { polyline, waypoints[] }
    this.selectedEntityId = null;

    // Model mapping
    this.defaultModels = {
      Fighter: "/models/f16-c_falcon.glb",
      Transport: "/models/aircraft.glb",
      Helicopter: "/models/helicopter.glb",
      Drone: "/models/autopilot_aircraft__drone.glb",
    };

    // ═══════════════════════════════════════════════════════════
    // Model heading offsets (radians)
    // Many glTF models have non-standard forward directions.
    // Adjust per model type if the visual orientation is wrong.
    //   0    = model forward = CesiumJS forward (standard glTF)
    //  +90°  = model faces +X in glTF, needs rotation to face -Z
    //  -90°  = model faces -X
    //  180°  = model faces +Z
    // ═══════════════════════════════════════════════════════════
    this.modelHeadingOffsets = {
      Fighter: CesiumMath.toRadians(0),
      Transport: CesiumMath.toRadians(0),
      Helicopter: CesiumMath.toRadians(0),
      Drone: CesiumMath.toRadians(0),
    };

    // Register pre-render callback for smooth dead-reckoning interpolation
    this._interpolationBound = this._interpolatePositions.bind(this);
    viewer.scene.preRender.addEventListener(this._interpolationBound);
  }

  /**
   * Apply full state sync — reconcile with current entities
   */
  applyFullState(stateMsg) {
    const aircraft = stateMsg.aircraft || {};
    const routes = stateMsg.routes || {};
    const selection = stateMsg.selection || {};

    // --- Aircraft ---
    const incomingIds = new Set(Object.keys(aircraft));

    // Remove entities no longer in state
    for (const [id] of this.aircraftEntities) {
      if (!incomingIds.has(id)) {
        this._removeAircraft(id);
      }
    }

    // Create or update entities
    for (const [id, acState] of Object.entries(aircraft)) {
      if (this.aircraftEntities.has(id)) {
        this._updateAircraft(id, acState);
      } else {
        this._createAircraft(id, acState);
      }
    }

    // --- Routes ---
    const incomingRouteIds = new Set(Object.keys(routes));
    for (const [id] of this.routeEntities) {
      if (!incomingRouteIds.has(id)) {
        this._removeRoute(id);
      }
    }
    for (const [id, rtState] of Object.entries(routes)) {
      this._renderRoute(id, rtState);
    }

    // --- Selection ---
    const selectedIds = selection.selectedIds || (selection.entityId ? [selection.entityId] : []);
    this._updateMultiSelection(selectedIds, selection.entityId || null);

    // CRITICAL: With requestRenderMode, Cesium won't re-render
    // unless we explicitly request it after programmatic changes
    this.viewer.scene.requestRender();
  }

  /**
   * Apply delta state — position/heading/speed updates with orientation
   */
  applyDelta(deltaMsg) {
    const aircraft = deltaMsg.aircraft || {};
    const selection = deltaMsg.selection || {};
    const now = performance.now();

    for (const [id, delta] of Object.entries(aircraft)) {
      const entry = this.aircraftEntities.get(id);
      if (!entry) continue;

      const pos = Cartesian3.fromDegrees(delta.lon, delta.lat, delta.alt);
      entry.entity.position = pos;

      // Update orientation (heading/pitch/roll) with model heading offset
      const modelOffset = this.modelHeadingOffsets[entry.modelType] || 0;
      const heading = CesiumMath.toRadians(delta.heading ?? 0) + modelOffset;
      const pitch = CesiumMath.toRadians(delta.pitch ?? 0);
      const roll = CesiumMath.toRadians(delta.roll ?? 0);
      const hpr = new HeadingPitchRoll(heading, pitch, roll);
      entry.entity.orientation = Transforms.headingPitchRollQuaternion(pos, hpr);

      // Store authoritative state for dead-reckoning interpolation
      entry.lastState = {
        lat: delta.lat,
        lon: delta.lon,
        alt: delta.alt,
        heading: delta.heading ?? 0,
        pitch: delta.pitch ?? 0,
        roll: delta.roll ?? 0,
        speed: delta.speed ?? 0,
        verticalSpeed: delta.verticalSpeed ?? 0,
        magneticHeading: delta.magneticHeading ?? 0,
        groundTrack: delta.groundTrack ?? 0,
        groundSpeed: delta.groundSpeed ?? 0,
        turnRate: delta.turnRate ?? 0,
        gLoad: delta.gLoad ?? 1,
        timestamp: now,
      };

      // Update label with full telemetry
      if (entry.label) {
        entry.label.position = pos;
        const hdg = Math.round(delta.heading ?? 0).toString().padStart(3, "0");
        const mag = Math.round(delta.magneticHeading ?? 0).toString().padStart(3, "0");
        const alt = Math.round(delta.alt ?? 0);
        const spd = Math.round(delta.speed ?? 0);
        const vs = delta.verticalSpeed ?? 0;
        const vsStr = vs > 0.5 ? `↑${Math.round(vs)}` : vs < -0.5 ? `↓${Math.abs(Math.round(vs))}` : "—";
        const r = (delta.roll ?? 0).toFixed(1);
        const p = (delta.pitch ?? 0).toFixed(1);
        entry.label.label.text =
          `${entry.callSign || id}\n` +
          `H${hdg}° M${mag}° ${alt}m\n` +
          `${spd}m/s ${vsStr} R${r}° P${p}°`;
      }

      // Trail update (reuse CallbackProperty created in _updateAircraft)
      entry.trailPositions.push(pos);
      if (entry.trailPositions.length > 500) {
        entry.trailPositions.shift();
      }
      if (!entry.trailEntity && entry.trailPositions.length > 2) {
        const positions = entry.trailPositions;
        entry.trailEntity = this.viewer.entities.add({
          id: `trail_${id}`,
          polyline: {
            positions: new CallbackProperty(() => positions, false),
            width: 2,
            material: Color.YELLOW.withAlpha(0.7),
            clampToGround: false,
          },
        });
      }
    }

    if (selection.entityId !== undefined) {
      const selectedIds = selection.selectedIds || (selection.entityId ? [selection.entityId] : []);
      this._updateMultiSelection(selectedIds, selection.entityId);
    }

    this.viewer.scene.requestRender();
  }

  /**
   * Handle commands from Qt
   */
  handleCommand(cmdMsg) {
    const type = cmdMsg.type;
    const payload = cmdMsg.payload || {};

    switch (type) {
      case "CMD_CREATE_ENTITY":
        this._createAircraft(payload.id, payload);
        break;
      case "CMD_REMOVE_ENTITY":
        this._removeAircraft(payload.id);
        break;
      case "CMD_SELECT_ENTITY":
        this._updateSelection(payload.entityId);
        break;
      case "CMD_HIGHLIGHT_ENTITY":
        this._updateSelection(payload.entityId);
        break;
      case "CMD_UPDATE_ROUTE":
        this._renderRoute(payload.id, payload);
        break;
    }
  }

  // ===== Private Methods =====

  _createAircraft(id, state) {
    const lon = state.lon || 0;
    const lat = state.lat || 0;
    const alt = state.alt || 0;
    const position = Cartesian3.fromDegrees(lon, lat, alt);
    const modelType = state.type || "Fighter";
    const modelUri =
      state.modelUri || this.defaultModels[modelType] || this.defaultModels.Fighter;

    // Heading/Pitch/Roll orientation with model heading offset
    const modelOffset = this.modelHeadingOffsets[modelType] || 0;
    const heading = CesiumMath.toRadians(state.heading ?? 0) + modelOffset;
    const pitch = CesiumMath.toRadians(state.pitch ?? 0);
    const roll = CesiumMath.toRadians(state.roll ?? 0);
    const hpr = new HeadingPitchRoll(heading, pitch, roll);
    const orientation = Transforms.headingPitchRollQuaternion(position, hpr);

    const entity = this.viewer.entities.add({
      id: `aircraft_${id}`,
      name: state.callSign || id,
      position: position,
      orientation: orientation,
      model: {
        uri: modelUri,
        minimumPixelSize: 32,
        scale: 1.0,
      },
    });

    const label = this.viewer.entities.add({
      id: `label_${id}`,
      position: position,
      label: {
        text: `${state.callSign || id}\n${alt.toFixed(0)}m`,
        font: "12pt monospace",
        style: LabelStyle.FILL_AND_OUTLINE,
        outlineWidth: 3,
        outlineColor: Color.fromCssColorString("#111723"),
        fillColor: Color.GHOSTWHITE,
        pixelOffset: new Cartesian2(0, -50),
        heightReference: HeightReference.NONE,
        disableDepthTestDistance: Number.POSITIVE_INFINITY,
        verticalOrigin: VerticalOrigin.BOTTOM,
      },
    });

    this.aircraftEntities.set(id, {
      entity,
      label,
      callSign: state.callSign || id,
      modelType: modelType,
      trailPositions: [],
      trailEntity: null,
      lastState: null, // for dead-reckoning interpolation
    });

    this.viewer.scene.requestRender();
    console.log(`[EntityRenderer] Created aircraft: ${id} (${state.callSign})`);
  }

  _updateAircraft(id, state) {
    const entry = this.aircraftEntities.get(id);
    if (!entry) return;

    const pos = Cartesian3.fromDegrees(state.lon, state.lat, state.alt || 0);
    entry.entity.position = pos;
    entry.callSign = state.callSign || id;

    // Update orientation with model heading offset
    const modelOffset = this.modelHeadingOffsets[entry.modelType] || 0;
    const heading = CesiumMath.toRadians(state.heading ?? 0) + modelOffset;
    const pitch = CesiumMath.toRadians(state.pitch ?? 0);
    const roll = CesiumMath.toRadians(state.roll ?? 0);
    const hpr = new HeadingPitchRoll(heading, pitch, roll);
    entry.entity.orientation = Transforms.headingPitchRollQuaternion(pos, hpr);

    if (entry.label) {
      entry.label.position = pos;
      const hdg = Math.round(state.heading ?? 0).toString().padStart(3, "0");
      const mag = Math.round(state.magneticHeading ?? 0).toString().padStart(3, "0");
      entry.label.label.text =
        `${state.callSign || id}\n` +
        `H${hdg}° M${mag}° ${Math.round(state.alt || 0)}m\n` +
        `${Math.round(state.speed || 0)}m/s`;
    }

    // Trail
    entry.trailPositions.push(pos);
    if (entry.trailPositions.length > 500) {
      entry.trailPositions.shift();
    }

    // Create trail polyline with CallbackProperty (avoids array copy per update)
    if (!entry.trailEntity && entry.trailPositions.length > 2) {
      const positions = entry.trailPositions;
      entry.trailEntity = this.viewer.entities.add({
        id: `trail_${id}`,
        polyline: {
          positions: new CallbackProperty(() => positions, false),
          width: 2,
          material: Color.YELLOW.withAlpha(0.7),
          clampToGround: false,
        },
      });
    }
  }

  _removeAircraft(id) {
    const entry = this.aircraftEntities.get(id);
    if (!entry) return;

    this.viewer.entities.removeById(`aircraft_${id}`);
    this.viewer.entities.removeById(`label_${id}`);
    if (entry.trailEntity) {
      this.viewer.entities.removeById(`trail_${id}`);
    }
    this.aircraftEntities.delete(id);
    this.viewer.scene.requestRender();
    console.log(`[EntityRenderer] Removed aircraft: ${id}`);
  }

  _renderRoute(routeId, routeState) {
    // Remove old route entities
    this._removeRoute(routeId);

    const waypoints = routeState.waypoints || [];
    if (waypoints.length < 2) return;

    const positions = waypoints.map((wp) =>
      Cartesian3.fromDegrees(wp.lon, wp.lat, wp.alt || 300)
    );

    const color = routeState.color
      ? Color.fromCssColorString(routeState.color)
      : Color.CYAN;

    // Polyline
    const polyline = this.viewer.entities.add({
      id: `route_line_${routeId}`,
      polyline: {
        positions: positions,
        width: 3,
        material: color.withAlpha(0.8),
        clampToGround: false,
      },
    });

    // Waypoint markers
    const wpEntities = waypoints.map((wp, idx) => {
      const colors = [
        Color.GREEN,
        Color.BLUE,
        Color.ORANGE,
        Color.PURPLE,
        Color.CYAN,
      ];
      return this.viewer.entities.add({
        id: `route_wp_${routeId}_${idx}`,
        name: wp.name || `WP${idx + 1}`,
        position: Cartesian3.fromDegrees(wp.lon, wp.lat, wp.alt || 300),
        point: {
          pixelSize: 12,
          color: colors[idx % colors.length],
          outlineColor: Color.WHITE,
          outlineWidth: 2,
          heightReference: HeightReference.NONE,
          disableDepthTestDistance: Number.POSITIVE_INFINITY,
        },
        label: {
          text: wp.name || `WP${idx + 1}`,
          font: "11pt monospace",
          style: LabelStyle.FILL_AND_OUTLINE,
          outlineWidth: 2,
          outlineColor: Color.fromCssColorString("#111723"),
          fillColor: Color.GHOSTWHITE,
          pixelOffset: new Cartesian2(0, -20),
          heightReference: HeightReference.NONE,
          disableDepthTestDistance: Number.POSITIVE_INFINITY,
        },
        properties: {
          routeId: routeId,
          waypointIndex: idx,
        },
      });
    });

    this.routeEntities.set(routeId, { polyline, wpEntities });
    this.viewer.scene.requestRender();
  }

  _removeRoute(routeId) {
    const entry = this.routeEntities.get(routeId);
    if (!entry) return;

    this.viewer.entities.removeById(`route_line_${routeId}`);
    entry.wpEntities.forEach((_, idx) => {
      this.viewer.entities.removeById(`route_wp_${routeId}_${idx}`);
    });
    this.routeEntities.delete(routeId);
  }

  _updateSelection(entityId) {
    this._updateMultiSelection(entityId ? [entityId] : [], entityId);
  }

  /**
   * Multi-selection highlighting:
   * - Primary (entityId): Yellow label, 1.5x scale, camera tracking
   * - Secondary (other selectedIds): Cyan label, 1.2x scale, no camera tracking
   * - Unselected: Ghost white label, 1.0x scale
   */
  _updateMultiSelection(selectedIds, primaryId) {
    const selectedSet = new Set(selectedIds || []);

    // De-highlight ALL previously highlighted entities
    for (const [id, entry] of this.aircraftEntities) {
      if (entry.label) {
        if (id === primaryId) {
          // Primary: Yellow + large
          entry.label.label.fillColor = Color.YELLOW;
          entry.label.label.scale = 1.5;
        } else if (selectedSet.has(id)) {
          // Secondary: Cyan + medium
          entry.label.label.fillColor = Color.CYAN;
          entry.label.label.scale = 1.2;
        } else {
          // Unselected: Ghost white + normal
          entry.label.label.fillColor = Color.GHOSTWHITE;
          entry.label.label.scale = 1.0;
        }
      }
    }

    this.selectedEntityId = primaryId;

    // Camera tracks primary entity only
    if (primaryId) {
      const entry = this.aircraftEntities.get(primaryId);
      if (entry && entry.entity) {
        this.viewer.trackedEntity = entry.entity;
      }
    }
  }

  // ═══════════════════════════════════════════════════════════
  // Dead-reckoning interpolation for smooth 60fps animation
  // Qt sends state at 20Hz; this fills the 3 frames between
  // each update using great-circle position extrapolation.
  // ═══════════════════════════════════════════════════════════
  _interpolatePositions() {
    const now = performance.now();
    let anyMoved = false;

    for (const [id, entry] of this.aircraftEntities) {
      if (!entry.lastState) continue;

      // Only interpolate selected entity for performance
      // Other aircraft update at bridge rate (~7Hz) which is smooth enough
      if (this.selectedEntityId && id !== this.selectedEntityId) continue;

      const state = entry.lastState;
      const elapsed = (now - state.timestamp) / 1000.0; // seconds

      // Only interpolate between ticks (5ms..150ms), not for stale data
      if (elapsed <= 0.005 || elapsed > 0.15 || state.speed < 1.0) continue;

      // Great-circle destination: extrapolate position
      const dist = state.speed * elapsed;
      const headingRad = state.heading * DEG_TO_RAD;
      const lat1 = state.lat * DEG_TO_RAD;
      const lon1 = state.lon * DEG_TO_RAD;
      const delta = dist / EARTH_RADIUS;

      const sinLat1 = Math.sin(lat1);
      const cosLat1 = Math.cos(lat1);
      const sinDelta = Math.sin(delta);
      const cosDelta = Math.cos(delta);

      const lat2 = Math.asin(
        sinLat1 * cosDelta + cosLat1 * sinDelta * Math.cos(headingRad)
      );
      const lon2 =
        lon1 +
        Math.atan2(
          Math.sin(headingRad) * sinDelta * cosLat1,
          cosDelta - sinLat1 * Math.sin(lat2)
        );

      const interpLat = lat2 * RAD_TO_DEG;
      const interpLon = lon2 * RAD_TO_DEG;
      const interpAlt = state.alt + state.verticalSpeed * elapsed;

      const pos = Cartesian3.fromDegrees(interpLon, interpLat, interpAlt);
      entry.entity.position = pos;

      // Orientation stays constant between ticks (heading/pitch/roll from last delta)
      const modelOffset = this.modelHeadingOffsets[entry.modelType] || 0;
      const heading = CesiumMath.toRadians(state.heading) + modelOffset;
      const pitch = CesiumMath.toRadians(state.pitch);
      const roll = CesiumMath.toRadians(state.roll);
      const hpr = new HeadingPitchRoll(heading, pitch, roll);
      entry.entity.orientation = Transforms.headingPitchRollQuaternion(pos, hpr);

      // Label follows
      if (entry.label) {
        entry.label.position = pos;
      }

      anyMoved = true;
    }

    if (anyMoved) {
      this.viewer.scene.requestRender();
    }
  }
}

export default EntityRenderer;
