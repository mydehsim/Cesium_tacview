/**
 * CameraController.js — Camera commands executed from Qt
 *
 * Handles CMD_CAMERA_FLY_TO, CMD_CAMERA_TRACK, CMD_CAMERA_ORBIT
 * from Qt bridge. No autonomous camera logic — Qt decides when to move.
 */

import {
  Cartesian3,
  Math as CesiumMath,
  HeadingPitchRange,
  BoundingSphere,
  Matrix4,
} from "cesium";

export class CameraController {
  constructor(viewer) {
    this.viewer = viewer;
    this.trackedEntityId = null;
    this._orbitTickRemover = null;
  }

  /**
   * Handle camera commands from Qt
   */
  handleCommand(cmdMsg) {
    const type = cmdMsg.type;
    const payload = cmdMsg.payload || {};

    switch (type) {
      case "CMD_CAMERA_FLY_TO":
        this.flyTo(payload.lat, payload.lon, payload.alt, payload.duration);
        break;
      case "CMD_CAMERA_TRACK":
        this.trackEntity(payload.entityId);
        break;
      case "CMD_CAMERA_STOP_TRACK":
        this.stopTracking();
        break;
      case "CMD_CAMERA_ORBIT":
        this.orbitEntity(payload.entityId, payload.radius);
        break;
      case "CMD_CAMERA_RESET":
        this.resetCamera();
        break;
    }
  }

  /**
   * Fly camera to a position
   */
  flyTo(lat, lon, alt = 5000, duration = 2) {
    this.stopTracking();
    this.viewer.camera.flyTo({
      destination: Cartesian3.fromDegrees(lon, lat, alt),
      orientation: {
        heading: CesiumMath.toRadians(0),
        pitch: CesiumMath.toRadians(-45),
      },
      duration: duration,
    });
  }

  /**
   * Track an aircraft entity (camera follows it)
   */
  trackEntity(entityId) {
    if (!entityId) return;

    const entity = this.viewer.entities.getById(`aircraft_${entityId}`);
    if (!entity) {
      console.warn(`[CameraController] Entity not found: aircraft_${entityId}`);
      return;
    }

    this.stopOrbit();
    this.viewer.trackedEntity = entity;
    this.trackedEntityId = entityId;
    console.log(`[CameraController] Tracking entity: ${entityId}`);
  }

  /**
   * Stop tracking any entity
   */
  stopTracking() {
    this.viewer.trackedEntity = undefined;
    this.viewer.camera.lookAtTransform(Matrix4.IDENTITY);
    this.trackedEntityId = null;
    this.stopOrbit();
  }

  /**
   * Orbit around an entity
   */
  orbitEntity(entityId, radius = 2000) {
    const entity = this.viewer.entities.getById(`aircraft_${entityId}`);
    if (!entity) return;

    this.stopOrbit();
    this.stopTracking();

    let angle = 0;
    const scene = this.viewer.scene;

    this._orbitTickRemover = scene.postRender.addEventListener(() => {
      const position = entity.position?.getValue(
        this.viewer.clock.currentTime
      );
      if (!position) return;

      angle += 0.005;
      const heading = angle;
      const pitch = CesiumMath.toRadians(-30);

      this.viewer.camera.lookAt(
        position,
        new HeadingPitchRange(heading, pitch, radius)
      );
    });
  }

  stopOrbit() {
    if (this._orbitTickRemover) {
      this._orbitTickRemover();
      this._orbitTickRemover = null;
      this.viewer.camera.lookAtTransform(Matrix4.IDENTITY);
    }
  }

  /**
   * Focus on entity (one-shot fly to its position)
   */
  focusEntity(entityId) {
    const entity = this.viewer.entities.getById(`aircraft_${entityId}`);
    if (!entity) return;

    const position = entity.position?.getValue(this.viewer.clock.currentTime);
    if (!position) return;

    this.viewer.camera.flyTo({
      destination: position,
      orientation: {
        heading: CesiumMath.toRadians(0),
        pitch: CesiumMath.toRadians(-45),
      },
      duration: 1.5,
    });
  }

  /**
   * Reset camera to default Istanbul overview
   */
  resetCamera() {
    this.stopTracking();
    this.viewer.camera.flyTo({
      destination: Cartesian3.fromDegrees(28.9784, 41.0082, 50000),
      orientation: {
        heading: CesiumMath.toRadians(0),
        pitch: CesiumMath.toRadians(-45),
      },
      duration: 2,
    });
  }
}

export default CameraController;
