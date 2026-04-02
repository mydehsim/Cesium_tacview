/**
 * EntityRenderer.js — Renders entities on Cesium map from Qt authoritative state
 *
 * Evolved from TrackVisualizer.js. This module:
 * - Creates/updates/removes aircraft entities based on state from Qt
 * - Renders waypoints, routes (polylines), labels, trails
 * - Highlights selected entity
 * - Does NOT hold any authoritative state — pure render slave
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
} from "cesium";

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
    this._updateSelection(selection.entityId || null);

    // CRITICAL: With requestRenderMode, Cesium won't re-render
    // unless we explicitly request it after programmatic changes
    this.viewer.scene.requestRender();
  }

  /**
   * Apply delta state — only position/heading/speed updates
   */
  applyDelta(deltaMsg) {
    const aircraft = deltaMsg.aircraft || {};
    const selection = deltaMsg.selection || {};

    for (const [id, delta] of Object.entries(aircraft)) {
      const entry = this.aircraftEntities.get(id);
      if (!entry) continue;

      const pos = Cartesian3.fromDegrees(delta.lon, delta.lat, delta.alt);
      entry.entity.position = pos;

      // Update orientation (heading/pitch/roll)
      const heading = CesiumMath.toRadians(delta.heading || 0);
      const pitch = CesiumMath.toRadians(delta.pitch || 0);
      const roll = CesiumMath.toRadians(delta.roll || 0);
      const hpr = new HeadingPitchRoll(heading, pitch, roll);
      entry.entity.orientation = Transforms.headingPitchRollQuaternion(pos, hpr);

      // Update label position + text
      if (entry.label) {
        entry.label.position = pos;
        entry.label.label.text = `${entry.callSign || id}\n${delta.alt?.toFixed(0) || 0}m | ${delta.speed?.toFixed(0) || 0}m/s`;
      }
    }

    if (selection.entityId !== undefined) {
      this._updateSelection(selection.entityId);
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
    const modelUri =
      state.modelUri || this.defaultModels[state.type] || this.defaultModels.Fighter;

    // Heading/Pitch/Roll orientation
    const heading = CesiumMath.toRadians(state.heading || 0);
    const pitch = CesiumMath.toRadians(state.pitch || 0);
    const roll = CesiumMath.toRadians(state.roll || 0);
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
      trailPositions: [],
      trailEntity: null,
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

    // Update orientation (heading/pitch/roll)
    const heading = CesiumMath.toRadians(state.heading || 0);
    const pitch = CesiumMath.toRadians(state.pitch || 0);
    const roll = CesiumMath.toRadians(state.roll || 0);
    const hpr = new HeadingPitchRoll(heading, pitch, roll);
    entry.entity.orientation = Transforms.headingPitchRollQuaternion(pos, hpr);

    if (entry.label) {
      entry.label.position = pos;
      entry.label.label.text = `${state.callSign || id}\n${(state.alt || 0).toFixed(0)}m | ${(state.speed || 0).toFixed(0)}m/s`;
    }

    // Trail
    entry.trailPositions.push(pos);
    if (entry.trailPositions.length > 500) {
      entry.trailPositions.shift();
    }

    // Create or update trail polyline
    if (!entry.trailEntity && entry.trailPositions.length > 2) {
      entry.trailEntity = this.viewer.entities.add({
        id: `trail_${id}`,
        polyline: {
          positions: entry.trailPositions,
          width: 2,
          material: Color.YELLOW.withAlpha(0.7),
          clampToGround: false,
        },
      });
    } else if (entry.trailEntity) {
      entry.trailEntity.polyline.positions = entry.trailPositions.slice();
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
    // De-highlight previous
    if (this.selectedEntityId) {
      const prev = this.aircraftEntities.get(this.selectedEntityId);
      if (prev && prev.label) {
        prev.label.label.fillColor = Color.GHOSTWHITE;
        prev.label.label.scale = 1.0;
      }
    }

    this.selectedEntityId = entityId;

    // Highlight new — enlarged yellow label + camera tracking
    if (entityId) {
      const entry = this.aircraftEntities.get(entityId);
      if (entry) {
        if (entry.label) {
          entry.label.label.fillColor = Color.YELLOW;
          entry.label.label.scale = 1.5;
        }
        // Track the selected entity (camera follows it)
        if (entry.entity) {
          this.viewer.trackedEntity = entry.entity;
        }
      }
    }
  }
}

export default EntityRenderer;
