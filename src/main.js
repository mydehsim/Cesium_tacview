import {
  Cartesian3,
  Cartesian2,
  Math as CesiumMath,
  Terrain,
  Viewer,
  createOsmBuildingsAsync,
  ImageryLayer,
  IonWorldImageryStyle,
  JulianDate,
  HeadingPitchRoll,
  Transforms,
  GeoJsonDataSource,
  HeadingPitchRange,
  VerticalOrigin,
  HorizontalOrigin,
  HeightReference,
  NearFarScalar,
  LabelStyle,
  Color,
  Ion,
  defined,
  ScreenSpaceEventType,
  ScreenSpaceEventHandler,
  Matrix4,
  CallbackProperty,
  Cartographic,
  Ellipsoid,
  EllipsoidGeodesic,
} from "cesium";
import "cesium/Build/Cesium/Widgets/widgets.css";
import "./style.css";

// Step 1.2: Add your Cesium ion access token
// See: https://cesium.com/learn/ion/cesium-ion-access-tokens/
// See: https://cesium.com/platform/cesium-ion/pricing/#frequently-asked-questions
Ion.defaultAccessToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiI4MzA4Mjc0MS1jY2M0LTRlYmQtYjc5My01OGQ4Yzk0OTMzMDAiLCJpZCI6MzU2NTM4LCJpYXQiOjE3NjIzMjU3NzB9.GnAL6LKzbzx6QcW8vprAwdkMHsWraP46l30QiQYduOU";

// Step 1.3: Initialize the Cesium Viewer in the HTML element with the
// `cesiumContainer` ID and visualize terrain
const viewer = new Viewer("cesiumContainer", {
  terrain: Terrain.fromWorldTerrain(),
  infoBox: false,
});

// Step 1.4: Add aerial imagery later with labels
const mapLayer = ImageryLayer.fromWorldImagery({
  style: IonWorldImageryStyle.AERIAL_WITH_LABELS,
});
viewer.imageryLayers.add(mapLayer);

// Step 1.5: Add Cesium OSM Buildings, a global 3D buildings layer.
const buildingTileset = await createOsmBuildingsAsync();
viewer.scene.primitives.add(buildingTileset);

// Step 1.6: Enable lighting the globe, set time of day, and turn on animation sped up 60x
viewer.scene.globe.enableLighting = true;
const customTime = JulianDate.fromDate(
  new Date(Date.UTC(2025, 5, 10, 3, 0, 0)),
);
viewer.clock.currentTime = customTime;
viewer.clock.shouldAnimate = true;
viewer.clock.multiplier = 60;

// Step 1.7: Fly the camera to balloon's initial position
function setCamera() {
  viewer.camera.lookAtTransform(Matrix4.IDENTITY);
  viewer.camera.flyTo({
    destination: Cartesian3.fromDegrees(28.9784, 41.0082, 5000), // Istanbul
    orientation: {
      heading: CesiumMath.toRadians(0.0),
      pitch: CesiumMath.toRadians(-45.0),
      range: 5000.0,
    },
    duration: 2,
  });
}
setCamera();

// ============================================================================
// ROUTE & BALLOON SYSTEM WITH GEODESIC CALCULATIONS
// ============================================================================

// Route configuration - all waypoints
let routeConfig = {
  waypoints: [
    { lat: 41.0082, lon: 28.9784, altitude: 300, name: "Istanbul" }, // Waypoint 1
    { lat: 37.9667, lon: 34.6781, altitude: 300, name: "Niğde" },   // Waypoint 2
  ],
  speedKmh: 100,
};

// Movement state
let isMoving = false;
let distanceOffsetMeters = 0.0;
let movementStartClock = viewer.clock.currentTime.clone();
let currentGeodesic = null;
let totalRouteDistance = 0;
let pathPositions = [];

// Tracked entities list for UI
const trackedEntities = [];

// Waypoint entities for dragging
let waypointEntities = [];

// Calculate geodesic route
function calculateGeodesicRoute() {
  const segments = [];
  const points = routeConfig.waypoints;

  if (points.length < 2) {
    console.warn("Need at least 2 waypoints for route");
    return segments;
  }

  for (let i = 0; i < points.length - 1; i++) {
    const startCarto = Cartographic.fromDegrees(
      points[i].lon,
      points[i].lat,
      points[i].altitude || 300
    );
    const endCarto = Cartographic.fromDegrees(
      points[i + 1].lon,
      points[i + 1].lat,
      points[i + 1].altitude || 300
    );

    const geodesic = new EllipsoidGeodesic(startCarto, endCarto);
    segments.push({
      geodesic,
      distance: geodesic.surfaceDistance,
      startCarto,
      endCarto,
    });
  }

  return segments;
}

// Get position along route at given distance
function getPositionAtDistance(distanceMeters, segments) {
  let remainingDistance = distanceMeters;

  for (const segment of segments) {
    if (remainingDistance <= segment.distance) {
      const fraction = remainingDistance / segment.distance;
      const interpCarto = segment.geodesic.interpolateUsingSurfaceDistance(
        remainingDistance
      );
      // Interpolate altitude between segment start and end
      const startAlt = segment.startCarto.height;
      const endAlt = segment.endCarto.height;
      interpCarto.height = startAlt + (endAlt - startAlt) * fraction;
      return Ellipsoid.WGS84.cartographicToCartesian(interpCarto);
    }
    remainingDistance -= segment.distance;
  }

  // End of route
  return Ellipsoid.WGS84.cartographicToCartesian(
    segments[segments.length - 1].endCarto
  );
}

// Initialize route
let routeSegments = calculateGeodesicRoute();
totalRouteDistance = routeSegments.reduce((sum, seg) => sum + seg.distance, 0);

// Create waypoint markers
function createWaypointMarkers() {
  // Clear existing waypoint entities
  waypointEntities.forEach(entity => viewer.entities.remove(entity));
  waypointEntities = [];

  routeConfig.waypoints.forEach((point, index) => {
    const colors = [Color.GREEN, Color.BLUE, Color.ORANGE, Color.PURPLE, Color.CYAN];
    const color = colors[index % colors.length];
    
    const entity = viewer.entities.add({
      id: `waypoint_${index}`,
      name: point.name || `Waypoint ${index + 1}`,
      position: Cartesian3.fromDegrees(point.lon, point.lat, point.altitude || 300),
      point: {
        pixelSize: 15,
        color: color,
        outlineColor: Color.WHITE,
        outlineWidth: 3,
        heightReference: HeightReference.NONE,
        disableDepthTestDistance: Number.POSITIVE_INFINITY,
      },
      label: {
        text: point.name || `WP${index + 1}`,
        font: "12pt monospace",
        style: LabelStyle.FILL_AND_OUTLINE,
        outlineWidth: 3,
        outlineColor: Color.fromCssColorString("#111723"),
        fillColor: Color.GHOSTWHITE,
        pixelOffset: new Cartesian2(0, -25),
        heightReference: HeightReference.NONE,
        disableDepthTestDistance: Number.POSITIVE_INFINITY,
      },
      properties: {
        waypointIndex: index
      }
    });

    waypointEntities.push(entity);
  });
}

createWaypointMarkers();

// Balloon position callback
const balloonPosition = new CallbackProperty(function (time, result) {
  // Default position (Istanbul) if no route
  if (!routeSegments.length) {
    const defaultPos = Cartesian3.fromDegrees(28.9784, 41.0082, 1000);
    return result ? Cartesian3.clone(defaultPos, result) : defaultPos;
  }

  const elapsed = JulianDate.secondsDifference(time, movementStartClock);
  const speedMps = (routeConfig.speedKmh * 1000) / 3600.0;
  const movingDistance = distanceOffsetMeters + (isMoving ? elapsed * speedMps : 0.0);

  const cartesian = getPositionAtDistance(movingDistance, routeSegments);

  pathPositions.push(cartesian);
  if (pathPositions.length > 3000) {
    pathPositions.shift();
  }

  return result ? Cartesian3.clone(cartesian, result) : cartesian;
}, false);

// Balloon orientation
const modelHeading = CesiumMath.toRadians(135);
const modelHPR = new HeadingPitchRoll(modelHeading, 0, 0);

// Balloon entity
const balloonEntity = viewer.entities.add({
  id: "CesiumBalloon",
  name: "Cesium Balloon",
  position: balloonPosition,
  orientation: new CallbackProperty(function (time) {
    const pos = balloonPosition.getValue(time);
    return Transforms.headingPitchRollQuaternion(pos, modelHPR);
  }, false),
  model: {
    uri: "/models/balloon.glb",
    minimumPixelSize: 32,
    runAnimations: true,
  },
  label: {
    text: new CallbackProperty(function (time) {
      const pos = balloonPosition.getValue(time);
      if (!pos) {
        return "Cesium Balloon\n(No route set)";
      }
      try {
        const carto = Ellipsoid.WGS84.cartesianToCartographic(pos);
        if (!carto) {
          return "Cesium Balloon\n(No route set)";
        }
        const lon = CesiumMath.toDegrees(carto.longitude).toFixed(5);
        const lat = CesiumMath.toDegrees(carto.latitude).toFixed(5);
        const height = carto.height.toFixed(1);
        const elapsed = JulianDate.secondsDifference(time, movementStartClock);
        const speedMps = (routeConfig.speedKmh * 1000) / 3600.0;
        const distance = distanceOffsetMeters + (isMoving ? elapsed * speedMps : 0.0);
        const distanceKm = (distance / 1000).toFixed(2);
        const totalKm = (totalRouteDistance / 1000).toFixed(2);
        const speed = isMoving ? `${routeConfig.speedKmh} km/h` : `stopped`;
        return `Cesium Balloon\nLat: ${lat}°\nLon: ${lon}°\nAlt: ${height} m\n${speed}\n${distanceKm}/${totalKm} km`;
      } catch (error) {
        return "Cesium Balloon\n(No route set)";
      }
    }, false),
    font: "14pt monospace",
    style: LabelStyle.FILL_AND_OUTLINE,
    outlineWidth: 4,
    outlineColor: Color.fromCssColorString("#111723"),
    fillColor: Color.GHOSTWHITE,
    pixelOffset: new Cartesian2(0, -80),
    heightReference: HeightReference.NONE,
  },
});

trackedEntities.push(balloonEntity);

// Path polyline
const pathEntity = viewer.entities.add({
  id: "CesiumBalloonPath",
  polyline: {
    positions: new CallbackProperty(function () {
      return pathPositions.slice();
    }, false),
    width: 3,
    material: Color.YELLOW.withAlpha(0.9),
    clampToGround: false,
  },
});

// Movement controls
function toggleMovement() {
  const now = viewer.clock.currentTime.clone();
  if (isMoving) {
    const elapsed = JulianDate.secondsDifference(now, movementStartClock);
    const speedMps = (routeConfig.speedKmh * 1000) / 3600.0;
    distanceOffsetMeters += elapsed * speedMps;
    isMoving = false;
    console.log("Balloon paused. Total meters:", distanceOffsetMeters.toFixed(1));
  } else {
    movementStartClock = now;
    isMoving = true;
    console.log("Balloon resumed.");
  }
}

// Keyboard shortcuts
document.addEventListener("keydown", function (e) {
  if (e.code === "KeyM") {
    toggleMovement();
  } else if (e.code === "KeyF") {
    viewer.trackedEntity = viewer.trackedEntity ? undefined : balloonEntity;
  }
});

console.log("Balloon initialized. Route:", routeConfig.start, "to", routeConfig.end);
console.log("Controls: 'M' to toggle movement, 'F' to toggle camera follow.");

// Step 2.2 Stream GeoJSON from a feature service
async function addGeoJson() {
  // Geojson url for South San Francisco Parks in public data portal https://data-southcity.opendata.arcgis.com/datasets/5851bfc2d1d445e3ac032b0a5f615313_0/explore
  const geojsonUrl =
    "https://services5.arcgis.com/inY93B27l4TSbT7h/arcgis/rest/services/SSF_Parks/FeatureServer/0/query?outFields=*&where=1%3D1&f=geojson";

  const dataSource = await GeoJsonDataSource.load(geojsonUrl, {
    clampToGround: true,
  });

  viewer.dataSources.add(dataSource);
  return dataSource;
}

const geoJsonDataSourceReference = await addGeoJson();

// Step 3.1 Use a color palette
// See https://colorbrewer2.org/#type=qualitative&scheme=Accent&n=6
function getCategoryColor(category) {
  const colorMap = {
    "Parks – City (developed)": "#a6cee3",
    "Parks – City (undeveloped/open space)": "#1f78b4",
    "Parks – City (trails)": "#b2df8a",
    "Parks (SSFUSD-owned sites)": "#33a02c",
    "Parks (other, privately owned)": "#fb9a99",
    default: "#CCCCCC",
  };

  return colorMap[category] || colorMap["default"];
}

mapLayer.saturation = 2.0;
mapLayer.contrast = 0.7;

// Step 3.2 Style a polygon 
const entities = geoJsonDataSourceReference.entities.values;
for (let i = 0; i < entities.length; i++) {
  const entity = entities[i];

  if (defined(entity.polygon)) {
    const category = entity.properties.Category.getValue(JulianDate.now());

    const color = Color.fromCssColorString(getCategoryColor(category));
    entity.polygon.material = color.withAlpha(0.8);
  }
}

// Step 3.3 Add label for a polygon (Alanın ortasında adı yazması ...)
function getPolygonCenter(entity) {
  const hierarchy = entity.polygon.hierarchy.getValue(JulianDate.now());
  const positions = hierarchy.positions;

  if (!positions || positions.length === 0) {
    return null;
  }

  const center = new Cartesian3(0, 0, 0);

  for (let i = 0; i < positions.length; i++) {
    Cartesian3.add(center, positions[i], center);
  }

  return Cartesian3.divideByScalar(center, positions.length, new Cartesian3());
}

for (let i = 0; i < entities.length; i++) {
  const entity = entities[i];
  if (defined(entity.polygon)) {
    const center = getPolygonCenter(entity);
    const category = entity.properties.Category.getValue(JulianDate.now());
    const color = Color.fromCssColorString(getCategoryColor(category));
    viewer.entities.add({
      position: center,
      point: {
        color: color,
        pixelSize: 18,
        outlineColor: Color.fromCssColorString("#111723"),
        outlineWidth: 3,
        heightReference: HeightReference.CLAMP_TO_GROUND,
        disableDepthTestDistance: Number.POSITIVE_INFINITY,
      },
      label: {
        text: entity.properties.FACID,
        font: "14pt monospace",
        heightReference: HeightReference.CLAMP_TO_GROUND,
        horizontalOrigin: HorizontalOrigin.LEFT,
        verticalOrigin: VerticalOrigin.BASELINE,
        fillColor: Color.GHOSTWHITE,
        outlineColor: Color.fromCssColorString("#111723"),
        outlineWidth: 8,
        style: LabelStyle.FILL_AND_OUTLINE,
        pixelOffset: new Cartesian2(15, 6),
        disableDepthTestDistance: Number.POSITIVE_INFINITY,
        scaleByDistance: new NearFarScalar(2000, 1.0, 22000, 0.3),
        translucencyByDistance: new NearFarScalar(12000, 1.0, 20000, 0.0),
      },
    });
  }
}

// Step 3.5 Handle Custom Picking (Farenin üzerinde bulunduğu noktanın bilgilerini gösterme)
function addCustomPicking() {
  const entity = viewer.entities.add({
    label: {
      show: false,
      showBackground: true,
      font: "14px monospace",
      backgroundColor: Color.fromCssColorString("#111723").withAlpha(0.8),
      backgroundPadding: new Cartesian2(16, 8),
      heightReference: HeightReference.CLAMP_TO_GROUND,
      pixelOffset: new Cartesian2(0, -50),
    },
  });

  const handler = new ScreenSpaceEventHandler(viewer.scene.canvas);

  // If the mouse is over a geojson entity from the parks dataset, show a label
  handler.setInputAction(function (movement) {
    const pickedObject = viewer.scene.pick(movement.endPosition);

    if (defined(pickedObject) && defined(pickedObject.id)) {
      if (geoJsonDataSourceReference.entities.contains(pickedObject.id)) {
        const cartesian = getPolygonCenter(pickedObject.id);
        entity.position = cartesian;
        entity.label.show = true;

        const parkType = pickedObject.id.properties.Class.getValue(
          JulianDate.now(),
        );
        const acreage = pickedObject.id.properties.Acres.getValue(
          JulianDate.now(),
        );
        entity.label.text = `Park Type: ${parkType}` + `\nAcres: ${acreage}`;
        return;
      }
    }
    entity.label.show = false;
  }, ScreenSpaceEventType.MOUSE_MOVE);
}
addCustomPicking();

// ============================================================================
// BALLOON CLICK HANDLER - Removed (buttons now in panel)
// ============================================================================

// ============================================================================
// DRAGGABLE WAYPOINTS ON MAP
// ============================================================================

let draggedEntity = null;
let isDragging = false;

const waypointDragHandler = new ScreenSpaceEventHandler(viewer.scene.canvas);

// Mouse down - start dragging
waypointDragHandler.setInputAction(function (click) {
  const pickedObject = viewer.scene.pick(click.position);
  if (defined(pickedObject) && defined(pickedObject.id)) {
    const entity = pickedObject.id;
    if (waypointEntities.includes(entity)) {
      isDragging = true;
      draggedEntity = entity;
      viewer.scene.screenSpaceCameraController.enableRotate = false;
      viewer.scene.screenSpaceCameraController.enableTranslate = false;
    }
  }
}, ScreenSpaceEventType.LEFT_DOWN);

// Mouse move - update position
waypointDragHandler.setInputAction(function (movement) {
  if (isDragging && draggedEntity) {
    const ray = viewer.camera.getPickRay(movement.endPosition);
    const cartesian = viewer.scene.globe.pick(ray, viewer.scene);
    
    if (defined(cartesian)) {
      draggedEntity.position = cartesian;
    }
  }
}, ScreenSpaceEventType.MOUSE_MOVE);

// Mouse up - finish dragging and update route
waypointDragHandler.setInputAction(function () {
  if (isDragging && draggedEntity) {
    const cartesian = draggedEntity.position.getValue(viewer.clock.currentTime);
    const carto = Ellipsoid.WGS84.cartesianToCartographic(cartesian);
    const lat = CesiumMath.toDegrees(carto.latitude);
    const lon = CesiumMath.toDegrees(carto.longitude);
    const alt = carto.height;

    const waypointIndex = draggedEntity.properties.waypointIndex.getValue();

    // Update route config
    if (routeConfig.waypoints[waypointIndex]) {
      routeConfig.waypoints[waypointIndex].lat = lat;
      routeConfig.waypoints[waypointIndex].lon = lon;
      routeConfig.waypoints[waypointIndex].altitude = alt;
      
      // Update UI
      renderWaypointList();
    }

    // Recalculate route
    routeSegments = calculateGeodesicRoute();
    totalRouteDistance = routeSegments.reduce((sum, seg) => sum + seg.distance, 0);
    
    // Reset movement
    isMoving = false;
    distanceOffsetMeters = 0.0;
    movementStartClock = viewer.clock.currentTime.clone();
    pathPositions = [];

    console.log(`Waypoint ${waypointIndex + 1} updated: ${lat.toFixed(4)}, ${lon.toFixed(4)}, ${alt.toFixed(1)}m`);
  }

  isDragging = false;
  draggedEntity = null;
  viewer.scene.screenSpaceCameraController.enableRotate = true;
  viewer.scene.screenSpaceCameraController.enableTranslate = true;
}, ScreenSpaceEventType.LEFT_UP);

// Step 4.1 Orbit a point when user holds down the Q key
let orbitHandler;

function toggleOrbit(position) {
  if (!defined(orbitHandler)) {
    orbitHandler = function (scene, time) {
      const pitch = CesiumMath.toRadians(-15);
      const range = 1000.0; // Distance from the point
      const delta = JulianDate.secondsDifference(time, viewer.clock.startTime);
      const newHeading = CesiumMath.toRadians(delta / 5); // degrees/sec

      viewer.camera.lookAt(
        position,
        new HeadingPitchRange(newHeading, pitch, range),
      );
    };
    viewer.scene.preRender.addEventListener(orbitHandler);
  } else {
    viewer.scene.preRender.removeEventListener(orbitHandler);
    orbitHandler = undefined;
    setCamera();
  }
}

document.addEventListener(
  "keydown",
  function (e) {
    if (typeof e.code !== "undefined") {
      if (e.code === "KeyQ") {
        const pos = balloonPosition.getValue(viewer.clock.currentTime);
        toggleOrbit(pos);
      }
    }
  },
  false,
);

// ============================================================================
// UI INTEGRATION - ENTITY SELECTOR WITH INTEGRATED INFO PANEL
// ============================================================================

// Track last selected entity to avoid unnecessary updates
let lastSelectedEntity = null;

// Update info panel on selection change
viewer.selectedEntityChanged.addEventListener(function () {
  if (viewer.selectedEntity !== lastSelectedEntity) {
    lastSelectedEntity = viewer.selectedEntity;
    updateEntitySelector();
  }
});

// Update periodically to refresh live data (but don't rebuild DOM)
setInterval(function() {
  if (viewer.selectedEntity) {
    updateEntityDetails();
  }
}, 100);

// Entity Selector Panel - lists all tracked entities with expandable details
function updateEntitySelector() {
  const entityList = document.getElementById("entityList");
  
  // Get current entity items
  const existingItems = Array.from(entityList.children);
  const currentEntityIds = trackedEntities.map(e => e.id);
  
  // Remove items that no longer exist
  existingItems.forEach(item => {
    const entityId = item.dataset.entityId;
    if (entityId && !currentEntityIds.includes(entityId)) {
      entityList.removeChild(item);
    }
  });
  
  // Update or create items
  trackedEntities.forEach((entity, index) => {
    let div = entityList.querySelector(`[data-entity-id="${entity.id}"]`);
    
    // Create new item if it doesn't exist
    if (!div) {
      div = document.createElement("div");
      div.className = "entity-item";
      div.dataset.entityId = entity.id;
      
      const name = document.createElement("div");
      name.className = "entity-item-name";
      name.textContent = entity.name;
      
      const info = document.createElement("div");
      info.className = "entity-item-info";
      info.textContent = entity.id;
      
      div.appendChild(name);
      div.appendChild(info);
      
      // Add click handler (only added once when item is created)
      div.addEventListener("click", function handleEntityClick(e) {
        // Stop event propagation
        e.stopPropagation();
        e.preventDefault();
        
        // Prevent click if clicking on delete button
        if (e.target.tagName === 'BUTTON' || e.target.closest('button')) {
          return;
        }
        
        // Toggle selection and tracking
        if (viewer.selectedEntity === entity) {
          // Deselect and unlock camera
          viewer.selectedEntity = undefined;
          viewer.trackedEntity = undefined;
          lastSelectedEntity = null;
          
          // Free camera movement
          viewer.camera.lookAtTransform(Matrix4.IDENTITY);
          
          console.log("Entity unlocked - camera free");
        } else {
          // Select and lock to entity
          viewer.selectedEntity = entity;
          viewer.trackedEntity = entity; // This locks camera to entity
          lastSelectedEntity = entity;
          
          // Initial zoom to entity - camera will stay locked to it
          viewer.zoomTo(entity, new HeadingPitchRange(
            0,                              // heading: 0 degrees
            CesiumMath.toRadians(-45),      // pitch: look down 45 degrees
            2000                            // range: 2km distance
          ));
          
          console.log("Entity locked:", entity.name);
        }
        
        // Small delay to prevent immediate re-selection
        setTimeout(() => {
          updateEntitySelector();
        }, 50);
      });
      
      entityList.appendChild(div);
    }
    
    // Update selected state
    if (viewer.selectedEntity === entity) {
      div.classList.add("selected");
      
      // Add or update details
      let details = div.querySelector(".entity-details");
      if (!details) {
        details = document.createElement("div");
        details.className = "entity-details";
        details.id = "entityDetailsContent";
        div.appendChild(details);
      }
      details.innerHTML = getEntityDetailsHTML(entity);
    } else {
      div.classList.remove("selected");
      
      // Remove details if not selected
      const details = div.querySelector(".entity-details");
      if (details) {
        div.removeChild(details);
      }
    }
  });
}

// Get entity details HTML (separate function for updates)
function getEntityDetailsHTML(entity) {
  const pos = entity.position.getValue(viewer.clock.currentTime);
  if (!pos) {
    return `<div class="entity-details-title">Entity Info</div>
            <div class="entity-details-row">No position data available</div>`;
  }
  const carto = Ellipsoid.WGS84.cartesianToCartographic(pos);
  const lon = CesiumMath.toDegrees(carto.longitude).toFixed(5);
  const lat = CesiumMath.toDegrees(carto.latitude).toFixed(5);
  const height = carto.height.toFixed(1);

  const elapsed = JulianDate.secondsDifference(
    viewer.clock.currentTime,
    movementStartClock
  );
  const speedMps = (routeConfig.speedKmh * 1000) / 3600.0;
  const distance = distanceOffsetMeters + (isMoving ? elapsed * speedMps : 0.0);
  const distanceKm = (distance / 1000).toFixed(2);
  const totalKm = (totalRouteDistance / 1000).toFixed(2);
  
  // Check if this entity can be deleted (not the balloon)
  const isDeletable = entity !== balloonEntity;
  const deleteButton = isDeletable 
    ? `<button onclick="deleteSelectedEntity()" style="background: #f44336; color: white; border: none; padding: 8px 16px; border-radius: 4px; cursor: pointer; width: 100%; margin-top: 10px; font-weight: bold;">🗑️ Delete Object</button>`
    : '';

  return `
    <div class="entity-details-title">Entity Info</div>
    <div class="entity-details-row">
      <span class="entity-details-label">Latitude:</span>
      <span class="entity-details-value">${lat}°</span>
    </div>
    <div class="entity-details-row">
      <span class="entity-details-label">Longitude:</span>
      <span class="entity-details-value">${lon}°</span>
    </div>
    <div class="entity-details-row">
      <span class="entity-details-label">Height:</span>
      <span class="entity-details-value">${height} m</span>
    </div>
    <div class="entity-details-row">
      <span class="entity-details-label">Speed:</span>
      <span class="entity-details-value">${isMoving ? routeConfig.speedKmh : 0} km/h</span>
    </div>
    <div class="entity-details-row">
      <span class="entity-details-label">Distance:</span>
      <span class="entity-details-value">${distanceKm} / ${totalKm} km</span>
    </div>
    <div class="entity-details-row">
      <span class="entity-details-label">Status:</span>
      <span class="entity-details-value">${isMoving ? "Moving" : "Stopped"}</span>
    </div>
    ${deleteButton}
  `;
}

// Update only the details content without rebuilding entire list
function updateEntityDetails() {
  const detailsContent = document.getElementById("entityDetailsContent");
  if (detailsContent && viewer.selectedEntity) {
    detailsContent.innerHTML = getEntityDetailsHTML(viewer.selectedEntity);
  }
}

// Delete selected entity from entity selector
window.deleteSelectedEntity = function() {
  const entity = viewer.selectedEntity;
  if (!entity || entity === balloonEntity) {
    return; // Don't delete balloon
  }
  
  // Find in created objects
  const objIndex = createdObjects.findIndex(obj => obj.entity === entity);
  if (objIndex > -1) {
    // Remove from viewer
    viewer.entities.remove(entity);
    
    // Remove from trackedEntities
    const trackedIndex = trackedEntities.indexOf(entity);
    if (trackedIndex > -1) {
      trackedEntities.splice(trackedIndex, 1);
    }
    
    // Remove from createdObjects
    createdObjects.splice(objIndex, 1);
    
    // Deselect
    viewer.selectedEntity = undefined;
    
    // Update UI
    renderObjectList();
    updateEntitySelector();
    
    console.log(`Deleted entity: ${entity.name}`);
  }
};

updateEntitySelector();

// ============================================================================
// CONTROL PANEL - NEW WAYPOINT SYSTEM
// ============================================================================

const speedInput = document.getElementById("speed");
const waypointList = document.getElementById("waypointList");
const addWaypointBtn = document.getElementById("addWaypoint");
const pickWaypointBtn = document.getElementById("pickWaypointPosition");
const applyRouteBtn = document.getElementById("applyRoute");
const startBtn = document.getElementById("startBtn");
const stopBtn = document.getElementById("stopBtn");
const resetBtn = document.getElementById("resetBtn");
const controlPanel = document.getElementById("controlPanel");
const toggleBtn = document.getElementById("toggleControlPanel");
const closeBtn = document.getElementById("closePanel");

// Object Management panel elements
const objectPanel = document.getElementById("objectPanel");
const toggleObjectBtn = document.getElementById("toggleObjectPanel");
const closeObjectBtn = document.getElementById("closeObjectPanel");
const objectTypeSelect = document.getElementById("objectType");
const objectNameInput = document.getElementById("objectName");
const objectLatInput = document.getElementById("objectLat");
const objectLonInput = document.getElementById("objectLon");
const objectAltInput = document.getElementById("objectAlt");
const objectColorInput = document.getElementById("objectColor");
const pickObjectBtn = document.getElementById("pickObjectPosition");
const createObjectBtn = document.getElementById("createObject");
const objectListDiv = document.getElementById("objectList");
const modelFileInput = document.getElementById("modelFile");
const modelScaleInput = document.getElementById("modelScale");
const modelSourceSelect = document.getElementById("modelSource");
const localModelSelect = document.getElementById("localModelSelect");
const objectGroupSelect = document.getElementById("objectGroup");
const createGroupBtn = document.getElementById("createGroup");
const groupListDiv = document.getElementById("groupList");

// Export/Import elements
const exportJSONBtn = document.getElementById("exportJSON");
const exportKMLBtn = document.getElementById("exportKML");
const importFileInput = document.getElementById("importFile");

// Created objects storage
let createdObjects = [];
let objectGroups = [];

// Toggle control panel
let isPanelOpen = false;
let isObjectPanelOpen = false;

function openPanel() {
  if (isObjectPanelOpen) {
    closeObjectPanel();
  }
  isPanelOpen = true;
  controlPanel.classList.add("open");
  toggleBtn.classList.add("panel-open");
  toggleObjectBtn.classList.add("panel-open");
}

function closePanel() {
  isPanelOpen = false;
  controlPanel.classList.remove("open");
  toggleBtn.classList.remove("panel-open");
  toggleObjectBtn.classList.remove("panel-open");
}

function openObjectPanel() {
  if (isPanelOpen) {
    closePanel();
  }
  isObjectPanelOpen = true;
  objectPanel.classList.add("open");
  toggleObjectBtn.classList.add("panel-open");
  toggleBtn.classList.add("panel-open");
}

function closeObjectPanel() {
  isObjectPanelOpen = false;
  objectPanel.classList.remove("open");
  toggleObjectBtn.classList.remove("panel-open");
  toggleBtn.classList.remove("panel-open");
}

toggleBtn.addEventListener("click", function () {
  if (isPanelOpen) {
    closePanel();
  } else {
    openPanel();
  }
});

closeBtn.addEventListener("click", function () {
  closePanel();
});

toggleObjectBtn.addEventListener("click", function () {
  if (isObjectPanelOpen) {
    closeObjectPanel();
  } else {
    openObjectPanel();
  }
});

closeObjectBtn.addEventListener("click", function () {
  closeObjectPanel();
});

// ============================================
// MAP POSITION PICKER FUNCTIONALITY
// ============================================
let isPickingPosition = false;
let pickingMode = null; // 'object' or 'waypoint'
let tempMarker = null;

function enablePositionPicker(mode) {
  isPickingPosition = true;
  pickingMode = mode;
  viewer.container.style.cursor = "crosshair";
  
  // Update button states
  if (mode === 'object') {
    pickObjectBtn.textContent = "❌ Cancel Pick";
    pickObjectBtn.style.background = "#f44336";
  } else if (mode === 'waypoint') {
    pickWaypointBtn.textContent = "❌ Cancel Pick";
    pickWaypointBtn.style.background = "#f44336";
  }
}

function disablePositionPicker() {
  isPickingPosition = false;
  pickingMode = null;
  viewer.container.style.cursor = "default";
  
  // Reset button states
  pickObjectBtn.textContent = "📍 Pick from Map";
  pickObjectBtn.style.background = "#4CAF50";
  pickWaypointBtn.textContent = "📍 Add Waypoint from Map";
  pickWaypointBtn.style.background = "#4CAF50";
  
  // Remove temp marker
  if (tempMarker) {
    viewer.entities.remove(tempMarker);
    tempMarker = null;
  }
}

// Map click handler for position picking
const mapPickHandler = new ScreenSpaceEventHandler(viewer.scene.canvas);
mapPickHandler.setInputAction(function(click) {
  if (!isPickingPosition) return;
  
  const ray = viewer.camera.getPickRay(click.position);
  const cartesian = viewer.scene.globe.pick(ray, viewer.scene);
  
  if (defined(cartesian)) {
    const cartographic = Ellipsoid.WGS84.cartesianToCartographic(cartesian);
    const lat = CesiumMath.toDegrees(cartographic.latitude);
    const lon = CesiumMath.toDegrees(cartographic.longitude);
    const alt = cartographic.height;
    
    // Show temp marker
    if (tempMarker) {
      viewer.entities.remove(tempMarker);
    }
    tempMarker = viewer.entities.add({
      position: cartesian,
      point: {
        pixelSize: 15,
        color: Color.YELLOW,
        outlineColor: Color.BLACK,
        outlineWidth: 2,
      },
      label: {
        text: `📍 Selected\nLat: ${lat.toFixed(5)}\nLon: ${lon.toFixed(5)}`,
        font: "12px sans-serif",
        style: LabelStyle.FILL_AND_OUTLINE,
        outlineWidth: 2,
        verticalOrigin: VerticalOrigin.BOTTOM,
        pixelOffset: new Cartesian2(0, -15),
        backgroundColor: Color.BLACK.withAlpha(0.7),
        showBackground: true,
        backgroundPadding: new Cartesian2(7, 5),
      }
    });
    
    if (pickingMode === 'object') {
      // Fill object position inputs
      objectLatInput.value = lat.toFixed(5);
      objectLonInput.value = lon.toFixed(5);
      objectAltInput.value = alt.toFixed(1);
    } else if (pickingMode === 'waypoint') {
      // Add waypoint directly
      addWaypoint(lat, lon, alt);
    }
    
    // Disable picker after selection
    disablePositionPicker();
  }
}, ScreenSpaceEventType.LEFT_CLICK);

// Pick position button handlers
pickObjectBtn.addEventListener("click", function() {
  if (isPickingPosition && pickingMode === 'object') {
    disablePositionPicker();
  } else {
    disablePositionPicker(); // Clear any other picking mode
    enablePositionPicker('object');
  }
});

pickWaypointBtn.addEventListener("click", function() {
  if (isPickingPosition && pickingMode === 'waypoint') {
    disablePositionPicker();
  } else {
    disablePositionPicker(); // Clear any other picking mode
    enablePositionPicker('waypoint');
  }
});

// ============================================
// END MAP POSITION PICKER
// ============================================

// Show/hide fields based on object type
objectTypeSelect.addEventListener("change", function() {
  const type = this.value;
  const modelFileGroup = document.getElementById("modelFileGroup");
  const modelScaleGroup = document.getElementById("modelScaleGroup");
  const colorGroup = document.getElementById("colorGroup");
  
  if (type === "model") {
    modelFileGroup.style.display = "block";
    modelScaleGroup.style.display = "block";
    colorGroup.style.display = "none";
    // Trigger model source change to show default option
    if (modelSourceSelect) {
      modelSourceSelect.dispatchEvent(new Event('change'));
    }
  } else {
    modelFileGroup.style.display = "none";
    modelScaleGroup.style.display = "none";
    colorGroup.style.display = "block";
    document.getElementById("modelUploadGroup").style.display = "none";
    document.getElementById("modelLocalGroup").style.display = "none";
  }
});

// Show/hide model source options
if (modelSourceSelect) {
  modelSourceSelect.addEventListener("change", function() {
    const source = this.value;
    const modelUploadGroup = document.getElementById("modelUploadGroup");
    const modelLocalGroup = document.getElementById("modelLocalGroup");
    
    if (source === "upload") {
      modelUploadGroup.style.display = "block";
      modelLocalGroup.style.display = "none";
    } else if (source === "local") {
      modelUploadGroup.style.display = "none";
      modelLocalGroup.style.display = "block";
    }
  });
}

// Open panel on page load (optional)
setTimeout(function() {
  openPanel();
}, 500);

// Waypoint list rendering
function renderWaypointList() {
  waypointList.innerHTML = "";
  
  routeConfig.waypoints.forEach((wp, index) => {
    const div = document.createElement("div");
    div.className = "waypoint-item";
    div.dataset.index = index;
    
    div.innerHTML = `
      <span class="waypoint-drag-handle" draggable="true">☰</span>
      <span class="waypoint-number">#${index + 1}</span>
      <div class="waypoint-inputs">
        <div class="waypoint-input-row">
          <input type="text" placeholder="Latitude" value="${wp.lat !== null ? wp.lat.toFixed(4) : ''}" class="wp-lat" />
          <input type="text" placeholder="Longitude" value="${wp.lon !== null ? wp.lon.toFixed(4) : ''}" class="wp-lon" />
        </div>
        <div class="waypoint-input-row">
          <input type="text" placeholder="Altitude (m)" value="${wp.altitude || ''}" class="wp-alt" />
        </div>
      </div>
      <button class="waypoint-remove">×</button>
    `;

    // Remove waypoint
    div.querySelector(".waypoint-remove").addEventListener("click", function () {
      if (routeConfig.waypoints.length <= 2) {
        alert("Need at least 2 waypoints for a route!");
        return;
      }
      routeConfig.waypoints.splice(index, 1);
      renderWaypointList();
      applyRouteConfiguration();
    });

    // Update waypoint on input change
    const inputs = div.querySelectorAll("input");
    inputs.forEach(input => {
      input.addEventListener("blur", function () {
        const lat = parseFloat(div.querySelector(".wp-lat").value);
        const lon = parseFloat(div.querySelector(".wp-lon").value);
        const alt = parseFloat(div.querySelector(".wp-alt").value) || 300;
        
        if (!isNaN(lat) && !isNaN(lon)) {
          routeConfig.waypoints[index] = { 
            lat, 
            lon, 
            altitude: alt,
            name: routeConfig.waypoints[index].name || `Waypoint ${index + 1}`
          };
          applyRouteConfiguration();
        }
      });
    });

    // Drag and drop for reordering - ONLY from drag handle
    const dragHandle = div.querySelector(".waypoint-drag-handle");
    
    dragHandle.addEventListener("dragstart", function (e) {
      div.classList.add("dragging");
      e.dataTransfer.effectAllowed = "move";
      e.dataTransfer.setData("text/plain", index);
    });

    dragHandle.addEventListener("dragend", function () {
      div.classList.remove("dragging");
    });

    div.addEventListener("dragover", function (e) {
      e.preventDefault();
      const dragging = document.querySelector(".dragging");
      if (!dragging) return;
      
      const afterElement = getDragAfterElement(waypointList, e.clientY);
      
      if (afterElement == null) {
        waypointList.appendChild(dragging);
      } else {
        waypointList.insertBefore(dragging, afterElement);
      }
    });

    div.addEventListener("drop", function (e) {
      e.preventDefault();
      const dragging = document.querySelector(".dragging");
      if (!dragging) return;
      
      // Get the current visual order from DOM
      const allItems = [...waypointList.querySelectorAll(".waypoint-item")];
      const newOrder = allItems.map(item => parseInt(item.dataset.index));
      
      // Reorder waypoints array according to visual order
      const reorderedWaypoints = newOrder.map(idx => routeConfig.waypoints[idx]);
      routeConfig.waypoints = reorderedWaypoints;
      
      renderWaypointList();
      applyRouteConfiguration();
    });

    waypointList.appendChild(div);
  });
}

function getDragAfterElement(container, y) {
  const draggableElements = [...container.querySelectorAll(".waypoint-item:not(.dragging)")];
  
  return draggableElements.reduce((closest, child) => {
    const box = child.getBoundingClientRect();
    const offset = y - box.top - box.height / 2;
    
    if (offset < 0 && offset > closest.offset) {
      return { offset: offset, element: child };
    } else {
      return closest;
    }
  }, { offset: Number.NEGATIVE_INFINITY }).element;
}

// Add new waypoint
addWaypointBtn.addEventListener("click", function () {
  routeConfig.waypoints.push({
    lat: null,
    lon: null,
    altitude: 300,
    name: `Waypoint ${routeConfig.waypoints.length + 1}`
  });
  renderWaypointList();
});

// Apply route configuration
function applyRouteConfiguration() {
  routeConfig.speedKmh = parseInt(speedInput.value) || 100;

  // Recalculate route
  routeSegments = calculateGeodesicRoute();
  totalRouteDistance = routeSegments.reduce((sum, seg) => sum + seg.distance, 0);

  // Recreate waypoint markers
  createWaypointMarkers();

  // Reset movement
  isMoving = false;
  distanceOffsetMeters = 0.0;
  movementStartClock = viewer.clock.currentTime.clone();
  pathPositions = [];

  // Fly camera to first waypoint
  if (routeConfig.waypoints.length > 0) {
    viewer.camera.flyTo({
      destination: Cartesian3.fromDegrees(
        routeConfig.waypoints[0].lon,
        routeConfig.waypoints[0].lat,
        50000
      ),
      duration: 2,
    });
  }

  console.log("Route updated. Total distance:", (totalRouteDistance / 1000).toFixed(2), "km");
}

applyRouteBtn.addEventListener("click", applyRouteConfiguration);

// Movement controls
startBtn.addEventListener("click", function () {
  if (!isMoving) {
    movementStartClock = viewer.clock.currentTime.clone();
    isMoving = true;
    console.log("Balloon started");
  }
});

stopBtn.addEventListener("click", function () {
  if (isMoving) {
    const now = viewer.clock.currentTime.clone();
    const elapsed = JulianDate.secondsDifference(now, movementStartClock);
    const speedMps = (routeConfig.speedKmh * 1000) / 3600.0;
    distanceOffsetMeters += elapsed * speedMps;
    isMoving = false;
    console.log("Balloon stopped at", (distanceOffsetMeters / 1000).toFixed(2), "km");
  }
});

resetBtn.addEventListener("click", function () {
  isMoving = false;
  distanceOffsetMeters = 0.0;
  movementStartClock = viewer.clock.currentTime.clone();
  pathPositions = [];
  console.log("Balloon reset to start");
});

// Initialize waypoint list on load
renderWaypointList();

// Apply initial route (Istanbul -> Niğde)
applyRouteConfiguration();

// Auto-select and track balloon entity after map loads (only once)
let initialBalloonSelectionDone = false;
viewer.scene.globe.tileLoadProgressEvent.addEventListener(function (remaining) {
  if (remaining === 0 && !initialBalloonSelectionDone) {
    initialBalloonSelectionDone = true;
    setTimeout(function() {
      viewer.selectedEntity = balloonEntity;
      viewer.trackedEntity = balloonEntity; // Lock camera to balloon
      console.log("Balloon tracked with Istanbul -> Niğde route. Press START to begin.");
    }, 1000);
  }
});

// ============================================================================
// OBJECT MANAGEMENT SYSTEM
// ============================================================================

// Render object list
function renderObjectList() {
  objectListDiv.innerHTML = "";
  
  createdObjects.forEach((obj, index) => {
    const div = document.createElement("div");
    div.className = "object-item";
    
    div.innerHTML = `
      <div>
        <div class="object-item-name">${obj.name}</div>
        <div class="object-item-type">${obj.type}</div>
      </div>
      <button class="btn-delete-object" data-index="${index}">Delete</button>
    `;
    
    // Select object on click
    div.addEventListener("click", function(e) {
      if (!e.target.classList.contains("btn-delete-object")) {
        viewer.selectedEntity = obj.entity;
        viewer.trackedEntity = obj.entity;
      }
    });
    
    objectListDiv.appendChild(div);
  });
  
  // Add delete button listeners
  document.querySelectorAll(".btn-delete-object").forEach(btn => {
    btn.addEventListener("click", function(e) {
      e.stopPropagation();
      const index = parseInt(this.dataset.index);
      const obj = createdObjects[index];
      
      // Remove entity from viewer
      viewer.entities.remove(obj.entity);
      
      // Remove from trackedEntities
      const trackedIndex = trackedEntities.indexOf(obj.entity);
      if (trackedIndex > -1) {
        trackedEntities.splice(trackedIndex, 1);
      }
      
      // Remove from array
      createdObjects.splice(index, 1);
      
      // Re-render list
      renderObjectList();
      updateEntitySelector();
    });
  });
}

// Create object
createObjectBtn.addEventListener("click", function() {
  const type = objectTypeSelect.value;
  const name = objectNameInput.value.trim() || `${type} ${createdObjects.length + 1}`;
  const lat = parseFloat(objectLatInput.value);
  const lon = parseFloat(objectLonInput.value);
  const alt = parseFloat(objectAltInput.value) || 0;
  const color = Color.fromCssColorString(objectColorInput.value);
  
  // Validate inputs
  if (isNaN(lat) || isNaN(lon)) {
    alert("Please enter valid latitude and longitude values");
    return;
  }
  
  if (lat < -90 || lat > 90) {
    alert("Latitude must be between -90 and 90");
    return;
  }
  
  if (lon < -180 || lon > 180) {
    alert("Longitude must be between -180 and 180");
    return;
  }
  
  let entity;
  const position = Cartesian3.fromDegrees(lon, lat, alt);
  
  // Create entity based on type
  switch(type) {
    case "point":
      entity = viewer.entities.add({
        name: name,
        position: position,
        point: {
          pixelSize: 12,
          color: color,
          outlineColor: Color.WHITE,
          outlineWidth: 2
        },
        label: {
          text: name,
          font: "14pt sans-serif",
          fillColor: Color.WHITE,
          outlineColor: Color.BLACK,
          outlineWidth: 2,
          style: LabelStyle.FILL_AND_OUTLINE,
          verticalOrigin: VerticalOrigin.BOTTOM,
          pixelOffset: new Cartesian2(0, -15)
        }
      });
      break;
      
    case "line":
      // For line, create a simple line from current position to 100km east
      const endPos = Cartesian3.fromDegrees(lon + 1, lat, alt);
      entity = viewer.entities.add({
        name: name,
        polyline: {
          positions: [position, endPos],
          width: 3,
          material: color,
          clampToGround: false
        }
      });
      break;
      
    case "polygon":
      // For polygon, create a simple square
      const offset = 0.1;
      entity = viewer.entities.add({
        name: name,
        polygon: {
          hierarchy: Cartesian3.fromDegreesArray([
            lon - offset, lat - offset,
            lon + offset, lat - offset,
            lon + offset, lat + offset,
            lon - offset, lat + offset
          ]),
          material: color.withAlpha(0.5),
          outline: true,
          outlineColor: Color.WHITE,
          outlineWidth: 2,
          height: alt
        }
      });
      break;
      
    case "model":
      const modelSource = modelSourceSelect ? modelSourceSelect.value : "upload";
      const scale = parseFloat(modelScaleInput.value) || 1;
      
      if (modelSource === "local") {
        // Use local model from project
        const modelUri = localModelSelect.value;
        
        console.log("Creating local model:", modelUri);
        
        entity = viewer.entities.add({
          name: name,
          position: position,
          model: {
            uri: modelUri,
            minimumPixelSize: 32,
            scale: scale
          },
          label: {
            text: name,
            font: "14pt sans-serif",
            fillColor: Color.WHITE,
            outlineColor: Color.BLACK,
            outlineWidth: 2,
            style: LabelStyle.FILL_AND_OUTLINE,
            verticalOrigin: VerticalOrigin.BOTTOM,
            pixelOffset: new Cartesian2(0, -40)
          }
        });
        
        // Store object with group info
        const group = objectGroupSelect.value;
        createdObjects.push({
          name: name,
          type: type,
          entity: entity,
          group: group,
          modelSource: "local",
          modelUri: modelUri
        });
        
        // Add to tracked entities for selector
        trackedEntities.push(entity);
        
        // Update UI
        renderObjectList();
        renderGroupList();
        updateEntitySelector();
        
        // Select and fly to new object
        viewer.selectedEntity = entity;
        viewer.flyTo(entity, {
          duration: 1.5,
          offset: new HeadingPitchRange(0, -45 * Math.PI / 180, 5000)
        });
        
        console.log(`Created 3D model: ${name} from local model ${modelUri}`);
        
        // Clear inputs
        objectNameInput.value = "";
        objectLatInput.value = "";
        objectLonInput.value = "";
        objectAltInput.value = "0";
        modelScaleInput.value = "1";
        
        return; // Exit - local model creation complete
        
      } else if (modelFileInput.files && modelFileInput.files[0]) {
        // Upload custom model file
        const file = modelFileInput.files[0];
        const reader = new FileReader();
        
        reader.onload = function(e) {
          const blob = new Blob([e.target.result]);
          const url = URL.createObjectURL(blob);
          
          entity = viewer.entities.add({
            name: name,
            position: position,
            model: {
              uri: url,
              minimumPixelSize: 32,
              scale: scale
            },
            label: {
              text: name,
              font: "14pt sans-serif",
              fillColor: Color.WHITE,
              outlineColor: Color.BLACK,
              outlineWidth: 2,
              style: LabelStyle.FILL_AND_OUTLINE,
              verticalOrigin: VerticalOrigin.BOTTOM,
              pixelOffset: new Cartesian2(0, -40)
            }
          });
          
          // Store object with group info
          const group = objectGroupSelect.value;
          createdObjects.push({
            name: name,
            type: type,
            entity: entity,
            group: group,
            modelFile: file.name
          });
          
          // Add to tracked entities for selector
          trackedEntities.push(entity);
          
          // Update UI
          renderObjectList();
          renderGroupList();
          updateEntitySelector();
          
          // Select and fly to new object
          viewer.selectedEntity = entity;
          viewer.flyTo(entity, {
            duration: 1.5,
            offset: new HeadingPitchRange(0, -45 * Math.PI / 180, 5000)
          });
          
          console.log(`Created 3D model: ${name} from ${file.name}`);
        };
        
        reader.readAsArrayBuffer(file);
        
        // Clear inputs
        objectNameInput.value = "";
        objectLatInput.value = "";
        objectLonInput.value = "";
        objectAltInput.value = "0";
        modelFileInput.value = "";
        modelScaleInput.value = "1";
        
        return; // Exit - async file loading in progress
        
      } else {
        alert("Please select a local model or upload a custom model file");
        return; // Exit - no model selected
      }
      // Note: This break is unreachable due to returns above
      break;
  }
  
  // Store object with group info
  const group = objectGroupSelect.value;
  createdObjects.push({
    name: name,
    type: type,
    entity: entity,
    group: group
  });
  
  // Add to tracked entities for selector
  trackedEntities.push(entity);
  
  // Clear inputs
  objectNameInput.value = "";
  objectLatInput.value = "";
  objectLonInput.value = "";
  objectAltInput.value = "0";
  
  // Update UI
  renderObjectList();
  renderGroupList();
  updateEntitySelector();
  
  // Select and fly to new object
  viewer.selectedEntity = entity;
  viewer.flyTo(entity, {
    duration: 1.5,
    offset: new HeadingPitchRange(0, -45 * Math.PI / 180, 5000)
  });
  
  console.log(`Created ${type}: ${name} at (${lat}, ${lon})`);
});

// Initialize object list
renderObjectList();

// ============================================================================
// GROUP MANAGEMENT SYSTEM
// ============================================================================

// Create new group
createGroupBtn.addEventListener("click", function() {
  const groupName = prompt("Enter group name:");
  if (groupName && groupName.trim()) {
    objectGroups.push({
      name: groupName.trim(),
      visible: true,
      color: Color.fromRandom()
    });
    renderGroupList();
    updateGroupSelect();
  }
});

// Render group list
function renderGroupList() {
  groupListDiv.innerHTML = "";
  
  objectGroups.forEach((group, index) => {
    const count = createdObjects.filter(obj => obj.group === group.name).length;
    
    const div = document.createElement("div");
    div.className = "group-item";
    
    div.innerHTML = `
      <div class="group-item-name">
        📁 ${group.name}
        <span class="group-item-count">(${count})</span>
      </div>
      <div class="group-item-actions">
        <button class="btn-toggle-group ${!group.visible ? 'hidden' : ''}" data-index="${index}">
          ${group.visible ? '👁️' : '🚫'}
        </button>
        <button class="btn-delete-group" data-index="${index}">Delete</button>
      </div>
    `;
    
    groupListDiv.appendChild(div);
  });
  
  // Add event listeners
  document.querySelectorAll(".btn-toggle-group").forEach(btn => {
    btn.addEventListener("click", function() {
      const index = parseInt(this.dataset.index);
      objectGroups[index].visible = !objectGroups[index].visible;
      
      // Toggle visibility of all objects in this group
      createdObjects.forEach(obj => {
        if (obj.group === objectGroups[index].name) {
          obj.entity.show = objectGroups[index].visible;
        }
      });
      
      renderGroupList();
    });
  });
  
  document.querySelectorAll(".btn-delete-group").forEach(btn => {
    btn.addEventListener("click", function() {
      const index = parseInt(this.dataset.index);
      const groupName = objectGroups[index].name;
      
      if (confirm(`Delete group "${groupName}"? Objects will remain but lose group assignment.`)) {
        // Remove group assignment from objects
        createdObjects.forEach(obj => {
          if (obj.group === groupName) {
            obj.group = "";
          }
        });
        
        objectGroups.splice(index, 1);
        renderGroupList();
        updateGroupSelect();
        renderObjectList();
      }
    });
  });
}

// Update group select dropdown
function updateGroupSelect() {
  const currentValue = objectGroupSelect.value;
  objectGroupSelect.innerHTML = '<option value="">No Group</option>';
  
  objectGroups.forEach(group => {
    const option = document.createElement("option");
    option.value = group.name;
    option.textContent = group.name;
    objectGroupSelect.appendChild(option);
  });
  
  objectGroupSelect.value = currentValue;
}

// ============================================================================
// EXPORT/IMPORT WAYPOINTS
// ============================================================================

// Export as JSON
exportJSONBtn.addEventListener("click", function() {
  const data = {
    speed: routeConfig.speedKmh,
    waypoints: routeConfig.waypoints
  };
  
  const json = JSON.stringify(data, null, 2);
  const blob = new Blob([json], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  
  const a = document.createElement("a");
  a.href = url;
  a.download = `route_${new Date().getTime()}.json`;
  a.click();
  
  URL.revokeObjectURL(url);
  console.log("Route exported as JSON");
});

// Export as KML
exportKMLBtn.addEventListener("click", function() {
  let kml = '<?xml version="1.0" encoding="UTF-8"?>\n';
  kml += '<kml xmlns="http://www.opengis.net/kml/2.2">\n';
  kml += '  <Document>\n';
  kml += '    <name>Balloon Route</name>\n';
  kml += '    <description>Route with waypoints</description>\n';
  
  // Add waypoints as placemarks
  routeConfig.waypoints.forEach((wp, index) => {
    if (wp.lat !== null && wp.lon !== null) {
      kml += '    <Placemark>\n';
      kml += `      <name>${wp.name || 'Waypoint ' + (index + 1)}</name>\n`;
      kml += '      <Point>\n';
      kml += `        <coordinates>${wp.lon},${wp.lat},${wp.altitude}</coordinates>\n`;
      kml += '      </Point>\n';
      kml += '    </Placemark>\n';
    }
  });
  
  // Add route line
  kml += '    <Placemark>\n';
  kml += '      <name>Route Path</name>\n';
  kml += '      <LineString>\n';
  kml += '        <coordinates>\n';
  routeConfig.waypoints.forEach(wp => {
    if (wp.lat !== null && wp.lon !== null) {
      kml += `          ${wp.lon},${wp.lat},${wp.altitude}\n`;
    }
  });
  kml += '        </coordinates>\n';
  kml += '      </LineString>\n';
  kml += '    </Placemark>\n';
  
  kml += '  </Document>\n';
  kml += '</kml>';
  
  const blob = new Blob([kml], { type: "application/vnd.google-earth.kml+xml" });
  const url = URL.createObjectURL(blob);
  
  const a = document.createElement("a");
  a.href = url;
  a.download = `route_${new Date().getTime()}.kml`;
  a.click();
  
  URL.revokeObjectURL(url);
  console.log("Route exported as KML");
});

// Import route
importFileInput.addEventListener("change", function(e) {
  const file = e.target.files[0];
  if (!file) return;
  
  const reader = new FileReader();
  
  reader.onload = function(event) {
    try {
      if (file.name.endsWith(".json")) {
        // Import JSON
        const data = JSON.parse(event.target.result);
        
        if (data.waypoints && Array.isArray(data.waypoints)) {
          routeConfig.waypoints = data.waypoints;
          if (data.speed) {
            routeConfig.speedKmh = data.speed;
            speedInput.value = data.speed;
          }
          
          renderWaypointList();
          applyRouteConfiguration();
          console.log("Route imported from JSON");
        }
      } else if (file.name.endsWith(".kml")) {
        // Import KML
        const parser = new DOMParser();
        const xmlDoc = parser.parseFromString(event.target.result, "text/xml");
        const placemarks = xmlDoc.getElementsByTagName("Placemark");
        
        const waypoints = [];
        for (let i = 0; i < placemarks.length; i++) {
          const name = placemarks[i].getElementsByTagName("name")[0]?.textContent || `Waypoint ${i + 1}`;
          const coords = placemarks[i].getElementsByTagName("coordinates")[0]?.textContent.trim();
          
          if (coords) {
            const [lon, lat, alt] = coords.split(",").map(s => parseFloat(s.trim()));
            if (!isNaN(lat) && !isNaN(lon)) {
              waypoints.push({
                lat: lat,
                lon: lon,
                altitude: alt || 300,
                name: name
              });
            }
          }
        }
        
        if (waypoints.length > 0) {
          routeConfig.waypoints = waypoints;
          renderWaypointList();
          applyRouteConfiguration();
          console.log("Route imported from KML");
        }
      }
    } catch (error) {
      console.error("Error importing file:", error);
      alert("Error importing file. Please check the file format.");
    }
  };
  
  reader.readAsText(file);
  
  // Clear input
  importFileInput.value = "";
});
