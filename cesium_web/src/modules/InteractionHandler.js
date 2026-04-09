/**
 * InteractionHandler.js — User interaction events sent to Qt
 *
 * Handles:
 * - Entity click (aircraft, waypoint)
 * - Map click (empty area)
 * - Waypoint drag (ScreenSpaceEventHandler)
 * - Context menu
 *
 * All interactions are sent as events to Qt via CesiumBridge.
 * No state is modified here — Qt decides what to do.
 */

import {
  defined,
  ScreenSpaceEventType,
  ScreenSpaceEventHandler,
  Ellipsoid,
  Math as CesiumMath,
} from "cesium";

export class InteractionHandler {
  constructor(viewer, bridge) {
    this.viewer = viewer;
    this.bridge = bridge;
    this.handler = new ScreenSpaceEventHandler(viewer.scene.canvas);
    this.dragHandler = new ScreenSpaceEventHandler(viewer.scene.canvas);

    this.draggedEntity = null;
    this.isDragging = false;

    this._setupClickHandler();
    this._setupDragHandler();

    console.log("[InteractionHandler] Initialized");
  }

  _setupClickHandler() {
    // Left click — entity selection or map click
    this.handler.setInputAction((click) => {
      const pickedObject = this.viewer.scene.pick(click.position);
      const isCtrl = click.ctrlKey === true;

      if (defined(pickedObject) && defined(pickedObject.id)) {
        const entity = pickedObject.id;
        const entityId = entity.id || "";

        // Determine event type based on Ctrl key
        const eventType = isCtrl ? "EVT_ENTITY_CTRL_CLICKED" : "EVT_ENTITY_CLICKED";

        let entityType = "unknown";
        if (entityId.startsWith("aircraft_")) {
          entityType = "aircraft";
          const id = entityId.replace("aircraft_", "");
          this.bridge.sendEvent(eventType, {
            entityId: id,
            entityType: entityType,
          });
        } else if (entityId.startsWith("label_")) {
          // Clicking label selects the aircraft
          entityType = "aircraft";
          const id = entityId.replace("label_", "");
          this.bridge.sendEvent(eventType, {
            entityId: id,
            entityType: entityType,
          });
        } else if (entityId.startsWith("route_wp_")) {
          entityType = "waypoint";
          const parts = entityId.replace("route_wp_", "").split("_");
          const routeId = parts.slice(0, -1).join("_");
          const wpIndex = parseInt(parts[parts.length - 1]);
          this.bridge.sendEvent("EVT_ENTITY_CLICKED", {
            entityId: entityId,
            entityType: entityType,
            routeId: routeId,
            waypointIndex: wpIndex,
          });
        } else {
          this.bridge.sendEvent("EVT_ENTITY_CLICKED", {
            entityId: entityId,
            entityType: entityType,
          });
        }
      } else {
        const ray = this.viewer.camera.getPickRay(click.position);
        const cartesian = this.viewer.scene.globe.pick(ray, this.viewer.scene);

        if (defined(cartesian)) {
          const carto = Ellipsoid.WGS84.cartesianToCartographic(cartesian);
          this.bridge.sendEvent("EVT_MAP_CLICKED", {
            lat: CesiumMath.toDegrees(carto.latitude),
            lon: CesiumMath.toDegrees(carto.longitude),
            alt: carto.height,
          });
        }
      }
    }, ScreenSpaceEventType.LEFT_CLICK);

    // Double click
    this.handler.setInputAction((click) => {
      const pickedObject = this.viewer.scene.pick(click.position);
      if (defined(pickedObject) && defined(pickedObject.id)) {
        const entityId = pickedObject.id.id || "";
        this.bridge.sendEvent("EVT_ENTITY_DOUBLE_CLICKED", {
          entityId: entityId,
        });
      }
    }, ScreenSpaceEventType.LEFT_DOUBLE_CLICK);

    // Right click (context menu)
    this.handler.setInputAction((click) => {
      const pickedObject = this.viewer.scene.pick(click.position);
      const ray = this.viewer.camera.getPickRay(click.position);
      const cartesian = this.viewer.scene.globe.pick(ray, this.viewer.scene);

      const payload = {};
      if (defined(pickedObject) && defined(pickedObject.id)) {
        payload.entityId = pickedObject.id.id || "";
      }
      if (defined(cartesian)) {
        const carto = Ellipsoid.WGS84.cartesianToCartographic(cartesian);
        payload.lat = CesiumMath.toDegrees(carto.latitude);
        payload.lon = CesiumMath.toDegrees(carto.longitude);
        payload.alt = carto.height;
      }
      payload.screenX = click.position.x;
      payload.screenY = click.position.y;

      this.bridge.sendEvent("EVT_CONTEXT_MENU", payload);
    }, ScreenSpaceEventType.RIGHT_CLICK);
  }

  _setupDragHandler() {
    this.dragHandler.setInputAction((click) => {
      const pickedObject = this.viewer.scene.pick(click.position);
      if (defined(pickedObject) && defined(pickedObject.id)) {
        const entity = pickedObject.id;
        const entityId = entity.id || "";

        if (entityId.startsWith("route_wp_")) {
          this.isDragging = true;
          this.draggedEntity = entity;
          this.viewer.scene.screenSpaceCameraController.enableRotate = false;
          this.viewer.scene.screenSpaceCameraController.enableTranslate = false;
        }
      }
    }, ScreenSpaceEventType.LEFT_DOWN);

    this.dragHandler.setInputAction((movement) => {
      if (this.isDragging && this.draggedEntity) {
        const ray = this.viewer.camera.getPickRay(movement.endPosition);
        const cartesian = this.viewer.scene.globe.pick(ray, this.viewer.scene);

        if (defined(cartesian)) {
          this.draggedEntity.position = cartesian;
        }
      }
    }, ScreenSpaceEventType.MOUSE_MOVE);

    this.dragHandler.setInputAction(() => {
      if (this.isDragging && this.draggedEntity) {
        // Get current position — handle both Property and raw Cartesian3
        let cartesian;
        const pos = this.draggedEntity.position;
        if (pos && typeof pos.getValue === "function") {
          cartesian = pos.getValue(this.viewer.clock.currentTime);
        } else {
          cartesian = pos;
        }

        if (defined(cartesian)) {
          const carto = Ellipsoid.WGS84.cartesianToCartographic(cartesian);
          const lat = CesiumMath.toDegrees(carto.latitude);
          const lon = CesiumMath.toDegrees(carto.longitude);
          const alt = carto.height;

          const entityId = this.draggedEntity.id || "";
          const parts = entityId.replace("route_wp_", "").split("_");
          const routeId = parts.slice(0, -1).join("_");
          const wpIndex = parseInt(parts[parts.length - 1]);

          this.bridge.sendEvent("EVT_WAYPOINT_MOVED", {
            routeId: routeId,
            waypointIndex: wpIndex,
            newPosition: { lat, lon, alt },
          });
        }
      }

      this.isDragging = false;
      this.draggedEntity = null;
      this.viewer.scene.screenSpaceCameraController.enableRotate = true;
      this.viewer.scene.screenSpaceCameraController.enableTranslate = true;
    }, ScreenSpaceEventType.LEFT_UP);
  }

  destroy() {
    this.handler.destroy();
    this.dragHandler.destroy();
  }
}

export default InteractionHandler;
