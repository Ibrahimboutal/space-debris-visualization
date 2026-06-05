// ==========================================
// AETHERIS // ORBITAL DEBRIS & RISK SYSTEM
// THREE.JS WEBGL SIMULATION & MATH ENGINE
// ==========================================

// Global configuration and state
const SIM_STATE = {
  timeScale: 100, // Simulation speed multiplier
  simTime: 0,     // Elapsed simulation time in seconds
  showDensityGrid: true,
  showOrbits: false,
  gridResolution: 32,
  filters: {
    regime: 'all',  // all, leo, meo, geo
    category: 'all', // all, sat, rocket, frag
    country: 'all'  // all, us, ru, cn, esa, other
  },
  selectedObject: null,
  debrisList: [],
  riskGrid: [],
  kesslerCount: 0,
  maxDebris: 2500
};

// Earth Constants
const EARTH_RADIUS_KM = 6371.0;
const MU = 3.986004418e5; // Earth's standard gravitational parameter (km^3/s^2)
const SCALE_FACTOR = 1.0 / EARTH_RADIUS_KM; // Earth = 1 unit in visual scene

// Scene variables
let scene, camera, renderer, controls;
let earthMesh, atmosphereMesh, sunLight;
let debrisParticles, debrisGeometry;
let gridPoints;
let orbitLine = null;
let lastTime = 0;

// Initialize components when window loads
window.addEventListener('DOMContentLoaded', () => {
  initThree();
  generateDebrisDataset();
  initHUD();
  animate(0);
});

// Create procedural high-tech canvas textures to avoid external asset loading (offline-safe & zero CORS issues)
function createProceduralEarthTextures() {
  const canvas = document.createElement('canvas');
  canvas.width = 2048;
  canvas.height = 1024;
  const ctx = canvas.getContext('2d');

  // Fill oceans
  ctx.fillStyle = '#060d1b';
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  // Draw cyber grid
  ctx.strokeStyle = 'rgba(14, 165, 233, 0.15)';
  ctx.lineWidth = 1;
  const numGridLines = 36;
  // Latitude lines
  for (let i = 1; i < numGridLines; i++) {
    const y = (canvas.height / numGridLines) * i;
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(canvas.width, y);
    ctx.stroke();
  }
  // Longitude lines
  for (let i = 0; i < numGridLines * 2; i++) {
    const x = (canvas.width / (numGridLines * 2)) * i;
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, canvas.height);
    ctx.stroke();
  }

  // Draw stylized vector continents (approximate coordinates for high-tech HUD look)
  ctx.fillStyle = '#092a4a';
  ctx.strokeStyle = '#0ea5e9';
  ctx.lineWidth = 2;

  // Function to draw a continent path
  function drawContinent(points, close = true) {
    ctx.beginPath();
    points.forEach((p, idx) => {
      const x = ((p[0] + 180) / 360) * canvas.width;
      const y = ((90 - p[1]) / 180) * canvas.height;
      if (idx === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    });
    if (close) ctx.closePath();
    ctx.fill();
    ctx.stroke();
  }

  // Simplified continent polygons
  const NorthAmerica = [
    [-168, 65], [-120, 70], [-80, 75], [-60, 60], [-50, 45],
    [-70, 20], [-90, 15], [-100, 18], [-105, 20], [-110, 8],
    [-85, 8], [-80, 25], [-85, 30], [-125, 33], [-125, 48], [-165, 54]
  ];

  const SouthAmerica = [
    [-80, 8], [-73, 10], [-45, -5], [-35, -7], [-40, -20],
    [-60, -40], [-72, -55], [-76, -53], [-70, -40], [-80, -15]
  ];

  const Africa = [
    [-17, 32], [10, 37], [32, 31], [34, 28], [51, 11], 
    [46, -4], [40, -34], [20, -34], [10, -15], [8, 5], [-15, 10]
  ];

  const Eurasia = [
    [-10, 60], [20, 70], [60, 75], [100, 77], [140, 72],
    [170, 65], [160, 40], [130, 35], [120, 15], [105, 20],
    [100, 5], [78, 8], [70, 22], [50, 25], [40, 15],
    [35, 30], [25, 36], [12, 43], [-10, 36]
  ];

  const Australia = [
    [113, -22], [115, -34], [135, -38], [150, -34],
    [152, -22], [140, -12], [130, -12], [120, -18]
  ];

  const Antarctica = [
    [-180, -75], [180, -75], [180, -90], [-180, -90]
  ];

  drawContinent(NorthAmerica);
  drawContinent(SouthAmerica);
  drawContinent(Africa);
  drawContinent(Eurasia);
  drawContinent(Australia);
  drawContinent(Antarctica);

  // Draw glowing city light dots (emissive simulation)
  const glowCanvas = document.createElement('canvas');
  glowCanvas.width = 1024;
  glowCanvas.height = 512;
  const gctx = glowCanvas.getContext('2d');
  gctx.fillStyle = '#000000';
  gctx.fillRect(0, 0, glowCanvas.width, glowCanvas.height);

  gctx.fillStyle = 'rgba(245, 158, 11, 0.9)'; // Amber glow lights
  // Draw some scatter points on land masses
  function drawCityLights(centerLat, centerLon, radius, count) {
    for (let i = 0; i < count; i++) {
      const lat = centerLat + (Math.random() - 0.5) * radius;
      const lon = centerLon + (Math.random() - 0.5) * radius;
      const x = ((lon + 180) / 360) * glowCanvas.width;
      const y = ((90 - lat) / 180) * glowCanvas.height;
      gctx.fillRect(x, y, 1.5, 1.5);
    }
  }

  // Major population zones
  drawCityLights(40, -95, 10, 40); // USA
  drawCityLights(50, 10, 8, 50);   // Europe
  drawCityLights(35, 115, 8, 60);  // China/Japan
  drawCityLights(20, 77, 6, 30);   // India
  drawCityLights(-25, -45, 8, 20); // Brazil
  drawCityLights(-30, 140, 5, 15); // SE Australia

  return {
    earthTex: new THREE.CanvasTexture(canvas),
    lightsTex: new THREE.CanvasTexture(glowCanvas)
  };
}

// Particle texture for debris and volumetric risk glow
function createParticleTexture() {
  const canvas = document.createElement('canvas');
  canvas.width = 64;
  canvas.height = 64;
  const ctx = canvas.getContext('2d');

  // Radial gradient for soft glow point
  const grad = ctx.createRadialGradient(32, 32, 0, 32, 32, 32);
  grad.addColorStop(0, 'rgba(255, 255, 255, 1)');
  grad.addColorStop(0.2, 'rgba(14, 165, 233, 0.8)');
  grad.addColorStop(0.5, 'rgba(14, 165, 233, 0.2)');
  grad.addColorStop(1, 'rgba(0, 0, 0, 0)');

  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, 64, 64);
  return new THREE.CanvasTexture(canvas);
}

// Voxel risk glow texture (soft radial dot)
function createVoxelGlowTexture() {
  const canvas = document.createElement('canvas');
  canvas.width = 64;
  canvas.height = 64;
  const ctx = canvas.getContext('2d');

  const grad = ctx.createRadialGradient(32, 32, 0, 32, 32, 32);
  grad.addColorStop(0, 'rgba(255, 255, 255, 0.9)');
  grad.addColorStop(0.3, 'rgba(239, 68, 68, 0.6)'); // Red core
  grad.addColorStop(0.7, 'rgba(239, 68, 68, 0.1)'); // Fading red
  grad.addColorStop(1, 'rgba(0, 0, 0, 0)');

  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, 64, 64);
  return new THREE.CanvasTexture(canvas);
}

// Setup Three.js environment
function initThree() {
  const container = document.getElementById('canvas-container');

  // Scene
  scene = new THREE.Scene();
  scene.fog = new THREE.FogExp2(0x030712, 0.015);

  // Camera
  camera = new THREE.PerspectiveCamera(45, container.clientWidth / container.clientHeight, 0.1, 100);
  camera.position.set(0, 2.5, 4.5);

  // Renderer
  renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setSize(container.clientWidth, container.clientHeight);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  container.appendChild(renderer.domElement);

  // Controls
  controls = new THREE.OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.dampingFactor = 0.05;
  controls.minDistance = 1.3;
  controls.maxDistance = 15;

  // Add lighting
  const ambientLight = new THREE.AmbientLight(0xffffff, 0.15);
  scene.add(ambientLight);

  sunLight = new THREE.DirectionalLight(0xffffff, 1.2);
  sunLight.position.set(5, 3, 5);
  scene.add(sunLight);

  // Earth Group (globe + glow)
  const earthGroup = new THREE.Group();
  scene.add(earthGroup);

  const { earthTex, lightsTex } = createProceduralEarthTextures();

  // Earth Mesh
  const earthGeo = new THREE.SphereGeometry(1.0, 64, 64);
  const earthMat = new THREE.MeshPhongMaterial({
    map: earthTex,
    emissiveMap: lightsTex,
    emissive: new THREE.Color('#ffaa44'),
    emissiveIntensity: 0.8,
    specular: new THREE.Color('#0e254a'),
    shininess: 15
  });
  earthMesh = new THREE.Mesh(earthGeo, earthMat);
  earthGroup.add(earthMesh);

  // Glow Atmosphere Shader
  const atmosphereGeo = new THREE.SphereGeometry(1.04, 32, 32);
  // Custom Atmosphere shader
  const atmosphereMat = new THREE.ShaderMaterial({
    vertexShader: `
      varying vec3 vNormal;
      void main() {
        vNormal = normalize(normalMatrix * normal);
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
      }
    `,
    fragmentShader: `
      varying vec3 vNormal;
      void main() {
        float intensity = pow(0.75 - dot(vNormal, vec3(0, 0, 1.0)), 2.5);
        gl_Position = vec4(0.0); // unused, keep output clean
        gl_FragColor = vec4(0.08, 0.64, 0.93, 1.0) * intensity * 0.7;
      }
    `,
    blending: THREE.AdditiveBlending,
    side: THREE.BackSide,
    transparent: true
  });

  // Simplified glowing Atmosphere using Standard Three.js material
  const simpleAtmosphereMat = new THREE.MeshBasicMaterial({
    color: 0x0ea5e9,
    transparent: true,
    opacity: 0.15,
    blending: THREE.AdditiveBlending,
    side: THREE.BackSide
  });

  atmosphereMesh = new THREE.Mesh(atmosphereGeo, simpleAtmosphereMat);
  earthGroup.add(atmosphereMesh);

  // Starfield
  const starGeo = new THREE.BufferGeometry();
  const starCount = 2000;
  const starPositions = new Float32Array(starCount * 3);
  for (let i = 0; i < starCount * 3; i += 3) {
    const r = 25 + Math.random() * 20;
    const theta = Math.random() * Math.PI * 2;
    const phi = Math.acos((Math.random() * 2) - 1);
    starPositions[i] = r * Math.sin(phi) * Math.cos(theta);
    starPositions[i + 1] = r * Math.sin(phi) * Math.sin(theta);
    starPositions[i + 2] = r * Math.cos(phi);
  }
  starGeo.setAttribute('position', new THREE.BufferAttribute(starPositions, 3));
  const starMat = new THREE.PointsMaterial({
    color: 0xffffff,
    size: 0.05,
    transparent: true,
    opacity: 0.7
  });
  const starPoints = new THREE.Points(starGeo, starMat);
  scene.add(starPoints);

  // Handle Resize
  window.addEventListener('resize', onWindowResize);
}

// Generate the space debris orbital parameters
function generateDebrisDataset() {
  const count = 2200;
  const countries = ['us', 'ru', 'cn', 'esa', 'other'];
  const categories = ['sat', 'rocket', 'frag'];

  for (let i = 0; i < count; i++) {
    // Generate semi-major axis (a) in km
    // LEO: 6500 to 8000 km, MEO: 10000 to 26000 km, GEO: 42000 to 42500 km
    let a, regime;
    const rVal = Math.random();
    if (rVal < 0.70) {
      a = 6600 + Math.random() * 1400; // LEO
      regime = 'leo';
    } else if (rVal < 0.90) {
      a = 9000 + Math.random() * 17000; // MEO
      regime = 'meo';
    } else {
      a = 41800 + Math.random() * 600; // GEO
      regime = 'geo';
    }

    // Eccentricity (e): mostly low, a few high
    let e = 0.0;
    if (Math.random() < 0.15) {
      e = 0.05 + Math.random() * 0.4;
    } else {
      e = Math.random() * 0.01;
    }

    // Inclination (i) in radians: polar, equatorial, or typical GTO
    let iRad;
    const iVal = Math.random();
    if (iVal < 0.4) {
      iRad = (50 + Math.random() * 48) * (Math.PI / 180); // LEO typical
    } else if (iVal < 0.7) {
      iRad = (Math.random() * 10) * (Math.PI / 180);      // Equatorial
    } else {
      iRad = 90 * (Math.PI / 180);                       // Polar
    }

    const raan = Math.random() * Math.PI * 2;
    const arg = Math.random() * Math.PI * 2;
    const ma = Math.random() * Math.PI * 2; // Mean Anomaly at epoch

    // Mean motion: n = sqrt(mu / a^3) rad/sec
    const n = Math.sqrt(MU / Math.pow(a, 3));

    // Meta details
    const cat = categories[Math.floor(Math.random() * categories.length)];
    const country = countries[Math.floor(Math.random() * countries.length)];
    const size = cat === 'sat' ? 1.5 + Math.random() * 5 : cat === 'rocket' ? 5 + Math.random() * 10 : 0.1 + Math.random() * 1.0;
    const catId = 20000 + i;

    let prefix = 'DEB-';
    if (cat === 'sat') prefix = 'SAT-';
    if (cat === 'rocket') prefix = 'RKB-';
    const name = `${prefix}${catId}`;

    SIM_STATE.debrisList.push({
      id: catId,
      name,
      a,
      e,
      i: iRad,
      raan,
      arg,
      ma,
      n,
      regime,
      category: cat,
      country,
      size,
      visible: true,
      position: new THREE.Vector3(),
      velocity: 0,
      altitude: 0,
      risk: 0
    });
  }

  // Create Visual Particles
  debrisGeometry = new THREE.BufferGeometry();
  const positions = new Float32Array(count * 3);
  const colors = new Float32Array(count * 3);
  const sizes = new Float32Array(count);

  debrisGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  debrisGeometry.setAttribute('color', new THREE.BufferAttribute(colors, 3));
  debrisGeometry.setAttribute('size', new THREE.BufferAttribute(sizes, 3));

  const particleTex = createParticleTexture();
  const debrisMaterial = new THREE.PointsMaterial({
    size: 0.045,
    vertexColors: true,
    map: particleTex,
    transparent: true,
    opacity: 0.9,
    blending: THREE.AdditiveBlending,
    depthWrite: false
  });

  debrisParticles = new THREE.Points(debrisGeometry, debrisMaterial);
  scene.add(debrisParticles);
}

// Analytical solver for Kepler's Equation: E - e*sin(E) = M
function solveKepler(M, e) {
  let E = M;
  const tolerance = 1e-6;
  const maxIterations = 30;

  for (let i = 0; i < maxIterations; i++) {
    const deltaE = (E - e * Math.sin(E) - M) / (1 - e * Math.cos(E));
    E -= deltaE;
    if (Math.abs(deltaE) < tolerance) break;
  }
  return E;
}

// Keplerian Orbit Propagator
function propagateOrbit(debris, simTime) {
  const { a, e, i, raan, arg, ma, n } = debris;

  // Mean Anomaly at sim time t
  const M = ma + n * simTime;

  // Solve Kepler's equation
  const E = solveKepler(M, e);

  // Position in the orbital plane
  const xOrb = a * (Math.cos(E) - e);
  const yOrb = a * Math.sqrt(1 - e * e) * Math.sin(E);

  // Rotation matrices coefficients (Orbital Plane to ECI Cartesian coordinates)
  const cosRaan = Math.cos(raan);
  const sinRaan = Math.sin(raan);
  const cosArg = Math.cos(arg);
  const sinArg = Math.sin(arg);
  const cosI = Math.cos(i);
  const sinI = Math.sin(i);

  // Position ECI (km)
  const x = xOrb * (cosRaan * cosArg - sinRaan * sinArg * cosI) - yOrb * (cosRaan * sinArg + sinRaan * cosArg * cosI);
  const y = xOrb * (sinRaan * cosArg + cosRaan * sinArg * cosI) - yOrb * (sinRaan * sinArg - cosRaan * cosArg * cosI);
  const z = xOrb * (sinArg * sinI) + yOrb * (cosArg * sinI);

  // Store computed values back to debris object
  debris.position.set(x, y, z);

  // Compute geodetic parameters
  const distance = debris.position.length();
  debris.altitude = distance - EARTH_RADIUS_KM;

  // Velocity (Vis-Viva equation): v = sqrt(mu * (2/r - 1/a))
  debris.velocity = Math.sqrt(MU * (2.0 / distance - 1.0 / a));

  // Visual position scaled
  const visualPos = debris.position.clone().multiplyScalar(SCALE_FACTOR);
  return visualPos;
}

// Initialize Volumetric Density Voxel Grid
function initDensityGrid() {
  const res = SIM_STATE.gridResolution;
  // Visual bounding range: [-3.0, 3.0] corresponds to roughly 19,000 km altitude (visual radius scale)
  const gridBound = 3.0;

  // Clear previous grid Points if any
  if (gridPoints) {
    scene.remove(gridPoints);
    gridPoints.geometry.dispose();
  }

  if (!SIM_STATE.showDensityGrid) return;

  // Build grid structure
  const gridData = new Float32Array(res * res * res);
  SIM_STATE.riskGrid = {
    res,
    bound: gridBound,
    data: gridData,
    voxelSize: (gridBound * 2) / res
  };

  // Create point cloud for drawing density
  const gridPositions = [];
  const gridColors = [];

  const step = (gridBound * 2) / res;
  for (let x = 0; x < res; x++) {
    for (let y = 0; y < res; y++) {
      for (let z = 0; z < res; z++) {
        const px = -gridBound + x * step + step / 2;
        const py = -gridBound + y * step + step / 2;
        const pz = -gridBound + z * step + step / 2;

        // Skip voxels inside or extremely close to Earth surface
        const distToCenter = Math.sqrt(px * px + py * py + pz * pz);
        if (distToCenter < 1.08) continue;

        gridPositions.push(px, py, pz);
        gridColors.push(0, 0, 0); // initial color is black/transparent
      }
    }
  }

  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute('position', new THREE.Float32BufferAttribute(gridPositions, 3));
  geometry.setAttribute('color', new THREE.Float32BufferAttribute(gridColors, 3));

  const voxelGlow = createVoxelGlowTexture();
  const material = new THREE.PointsMaterial({
    size: 0.18,
    vertexColors: true,
    map: voxelGlow,
    transparent: true,
    opacity: 0.65,
    blending: THREE.AdditiveBlending,
    depthWrite: false
  });

  gridPoints = new THREE.Points(geometry, material);
  scene.add(gridPoints);
}

// Map orbital points into volumetric voxels and update grid colors
function updateDensityGrid() {
  if (!SIM_STATE.showDensityGrid || !SIM_STATE.riskGrid.data) return;

  const { res, bound, data, voxelSize } = SIM_STATE.riskGrid;

  // Clear previous data
  data.fill(0);

  // Bin particles
  let visibleActiveCount = 0;
  SIM_STATE.debrisList.forEach(deb => {
    if (!deb.visible) return;
    visibleActiveCount++;

    const pos = deb.position.clone().multiplyScalar(SCALE_FACTOR); // visual coords
    
    // Check if within bounds
    if (Math.abs(pos.x) < bound && Math.abs(pos.y) < bound && Math.abs(pos.z) < bound) {
      const ix = Math.floor((pos.x + bound) / voxelSize);
      const iy = Math.floor((pos.y + bound) / voxelSize);
      const iz = Math.floor((pos.z + bound) / voxelSize);

      if (ix >= 0 && ix < res && iy >= 0 && iy < res && iz >= 0 && iz < res) {
        const index = ix + iy * res + iz * res * res;
        data[index] += 1.0;
      }
    }
  });

  // Calculate voxel colors based on debris density
  const colorsAttr = gridPoints.geometry.attributes.color;
  const positionsAttr = gridPoints.geometry.attributes.position;
  const colors = colorsAttr.array;
  const positions = positionsAttr.array;
  const numPoints = positions.length / 3;

  let peakDensity = 0;
  let maxVoxelIndex = 0;

  for (let i = 0; i < numPoints; i++) {
    const px = positions[i * 3];
    const py = positions[i * 3 + 1];
    const pz = positions[i * 3 + 2];

    const ix = Math.floor((px + bound) / voxelSize);
    const iy = Math.floor((py + bound) / voxelSize);
    const iz = Math.floor((pz + bound) / voxelSize);

    let density = 0;
    if (ix >= 0 && ix < res && iy >= 0 && iy < res && iz >= 0 && iz < res) {
      const index = ix + iy * res + iz * res * res;
      density = data[index];
    }

    if (density > peakDensity) {
      peakDensity = density;
      maxVoxelIndex = i;
    }

    // Color scaling
    if (density === 0) {
      colors[i * 3] = 0;     // R
      colors[i * 3 + 1] = 0; // G
      colors[i * 3 + 2] = 0; // B
    } else {
      // High density grid cells glow red, medium orange, low yellow/faded
      if (density > 8) {
        colors[i * 3] = 0.95;     // R
        colors[i * 3 + 1] = 0.25; // G
        colors[i * 3 + 2] = 0.25; // B
      } else if (density > 4) {
        colors[i * 3] = 0.95;     // R
        colors[i * 3 + 1] = 0.60; // G
        colors[i * 3 + 2] = 0.15; // B
      } else {
        colors[i * 3] = 0.14;     // R
        colors[i * 3 + 1] = 0.65; // G
        colors[i * 3 + 2] = 0.93; // B
      }
    }
  }

  colorsAttr.needsUpdate = true;

  // Set the selected debris risk factor if one is active
  if (SIM_STATE.selectedObject) {
    const pos = SIM_STATE.selectedObject.position.clone().multiplyScalar(SCALE_FACTOR);
    const ix = Math.floor((pos.x + bound) / voxelSize);
    const iy = Math.floor((pos.y + bound) / voxelSize);
    const iz = Math.floor((pos.z + bound) / voxelSize);

    let localDensity = 0;
    if (ix >= 0 && ix < res && iy >= 0 && iy < res && iz >= 0 && iz < res) {
      localDensity = data[ix + iy * res + iz * res * res];
    }

    // Normalize risk score to percentage
    SIM_STATE.selectedObject.risk = Math.min(localDensity * 12.5, 99.9);
  }

  // Set system status based on peakDensity
  const statusText = document.getElementById('sys-status-text');
  const statusDot = document.getElementById('sys-status-dot');
  const riskVal = document.getElementById('risk-index-indicator');

  if (peakDensity > 18) {
    statusText.innerText = 'DANGER: HIGH COLLISION RISK';
    statusDot.className = 'indicator danger';
    riskVal.innerText = 'CRITICAL';
    riskVal.className = 'info-value red';
  } else if (peakDensity > 10) {
    statusText.innerText = 'CONJUNCTION ALERT';
    statusDot.className = 'indicator warning';
    riskVal.innerText = 'MEDIUM';
    riskVal.className = 'info-value amber';
  } else {
    statusText.innerText = 'NOMINAL';
    statusDot.className = 'indicator pulse';
    riskVal.innerText = 'LOW';
    riskVal.className = 'info-value emerald';
  }
}

// Generate the visual representation of debris (particles buffer)
function updateDebrisParticles() {
  const positionsAttr = debrisGeometry.attributes.position;
  const colorsAttr = debrisGeometry.attributes.color;
  const sizesAttr = debrisGeometry.attributes.size;

  const positions = positionsAttr.array;
  const colors = colorsAttr.array;
  const sizes = sizesAttr.array;

  let activeCount = 0;

  for (let i = 0; i < SIM_STATE.debrisList.length; i++) {
    const deb = SIM_STATE.debrisList[i];

    // Evaluate visibility filters
    let meetsFilter = true;

    // Orbit regime filter
    if (SIM_STATE.filters.regime !== 'all' && deb.regime !== SIM_STATE.filters.regime) {
      meetsFilter = false;
    }

    // Category filter
    if (SIM_STATE.filters.category !== 'all' && deb.category !== SIM_STATE.filters.category) {
      meetsFilter = false;
    }

    // Country filter
    if (SIM_STATE.filters.country !== 'all' && deb.country !== SIM_STATE.filters.country) {
      meetsFilter = false;
    }

    deb.visible = meetsFilter;

    if (meetsFilter) {
      // Propagate orbit
      const visualPos = propagateOrbit(deb, SIM_STATE.simTime);
      
      positions[i * 3] = visualPos.x;
      positions[i * 3 + 1] = visualPos.y;
      positions[i * 3 + 2] = visualPos.z;

      // Color mapping: active satellites = cyan, rocket stages = amber, fragments = red/gray
      if (deb.category === 'sat') {
        colors[i * 3] = 0.08;     // R
        colors[i * 3 + 1] = 0.65; // G
        colors[i * 3 + 2] = 0.93; // B
        sizes[i] = 0.065;
      } else if (deb.category === 'rocket') {
        colors[i * 3] = 0.96;     // R
        colors[i * 3 + 1] = 0.62; // G
        colors[i * 3 + 2] = 0.04; // B
        sizes[i] = 0.085;
      } else {
        colors[i * 3] = 0.93;     // R
        colors[i * 3 + 1] = 0.27; // G
        colors[i * 3 + 2] = 0.27; // B
        sizes[i] = 0.045;
      }

      // Selection highlight
      if (SIM_STATE.selectedObject && SIM_STATE.selectedObject.id === deb.id) {
        colors[i * 3] = 1.0;
        colors[i * 3 + 1] = 1.0;
        colors[i * 3 + 2] = 1.0;
        sizes[i] = 0.15; // Larger highlighted size
      }

      activeCount++;
    } else {
      // Hide particle far away
      positions[i * 3] = 9999;
      positions[i * 3 + 1] = 9999;
      positions[i * 3 + 2] = 9999;
    }
  }

  positionsAttr.needsUpdate = true;
  colorsAttr.needsUpdate = true;
  sizesAttr.needsUpdate = true;

  document.getElementById('debris-count-val').innerText = activeCount;
}

// Dynamic linked-view altitude charts
function updateAnalyticsCharts() {
  const altChartContainer = document.getElementById('altitude-chart');
  
  // Calculate histogram data
  const numBuckets = 10;
  const buckets = new Array(numBuckets).fill(0);
  const bucketLimit = 36000.0; // altitude cap to fit histogram (up to GEO)

  SIM_STATE.debrisList.forEach(deb => {
    if (!deb.visible) return;
    const bucketIdx = Math.min(
      Math.floor((deb.altitude / bucketLimit) * numBuckets),
      numBuckets - 1
    );
    if (bucketIdx >= 0) {
      buckets[bucketIdx]++;
    }
  });

  const maxVal = Math.max(...buckets, 1);

  // Re-generate chart bars dynamically (cleaner, premium look, and zero dependency issues)
  altChartContainer.innerHTML = '';
  buckets.forEach((count, idx) => {
    const percentage = (count / maxVal) * 100;
    const bar = document.createElement('div');
    bar.className = 'chart-bar';
    bar.style.height = `${percentage}%`;

    // Vary colors slightly per altitude
    if (idx < 2) {
      bar.classList.add('red'); // LEO (Highly congested)
    } else if (idx < 6) {
      bar.classList.add('amber'); // MEO
    }
    
    const minAlt = Math.round((idx * bucketLimit) / numBuckets);
    const maxAlt = Math.round(((idx + 1) * bucketLimit) / numBuckets);
    bar.setAttribute('data-tooltip', `${minAlt}-${maxAlt}km: ${count} objs`);
    altChartContainer.appendChild(bar);
  });
}

// Raycaster listener to click and select orbital particles
window.addEventListener('click', (event) => {
  // Only register clicks on canvas-container
  if (event.target.tagName !== 'CANVAS') return;

  const container = document.getElementById('canvas-container');
  const rect = renderer.domElement.getBoundingClientRect();
  const mouse = new THREE.Vector2(
    ((event.clientX - rect.left) / container.clientWidth) * 2 - 1,
    -((event.clientY - rect.top) / container.clientHeight) * 2 + 1
  );

  const raycaster = new THREE.Raycaster();
  raycaster.params.Points.threshold = 0.08; // Click margin
  raycaster.setFromCamera(mouse, camera);

  const intersects = raycaster.intersectObject(debrisParticles);

  if (intersects.length > 0) {
    const index = intersects[0].index;
    const targetObj = SIM_STATE.debrisList[index];
    if (targetObj && targetObj.visible) {
      SIM_STATE.selectedObject = targetObj;
      logConsoleMessage(`Selected Target: ${targetObj.name} (CatID ${targetObj.id})`, 'info');
      
      // Draw its trajectory line
      drawOrbitLine(targetObj);
      return;
    }
  }

  // If we clicked empty space, clear selection
  SIM_STATE.selectedObject = null;
  clearOrbitLine();
});

// Trace full orbit loop for selected object
function drawOrbitLine(debris) {
  clearOrbitLine();

  const numPoints = 120;
  const points = [];
  
  // Propagate orbit full circle (Mean anomaly 0 to 2pi)
  const originalMa = debris.ma;
  const tempDebris = { ...debris };

  for (let idx = 0; idx <= numPoints; idx++) {
    tempDebris.ma = (idx / numPoints) * Math.PI * 2;
    // We compute relative visual coordinates for simTime = 0 with adjusted mean anomaly
    const pos = propagateOrbit(tempDebris, 0);
    points.push(pos);
  }

  const geometry = new THREE.BufferGeometry().setFromPoints(points);
  const material = new THREE.LineBasicMaterial({
    color: 0x0ea5e9,
    linewidth: 1.5,
    transparent: true,
    opacity: 0.8
  });

  orbitLine = new THREE.LineLoop(geometry, material);
  scene.add(orbitLine);
}

function clearOrbitLine() {
  if (orbitLine) {
    scene.remove(orbitLine);
    orbitLine.geometry.dispose();
    orbitLine = null;
  }
}

// Kessler Syndrome Simulator Trigger
function triggerKesslerCascade() {
  SIM_STATE.kesslerCount++;

  // Find a high-density zone or default to active selection LEO orbit crossing
  let impactPoint = new THREE.Vector3(1.15, 0.05, 0.2); // Default collision point in visual coordinates
  let baseVelocity = 7.5; // typical km/s orbital speed

  if (SIM_STATE.selectedObject) {
    impactPoint = SIM_STATE.selectedObject.position.clone().multiplyScalar(SCALE_FACTOR);
    baseVelocity = SIM_STATE.selectedObject.velocity;
  } else {
    // Pick a random LEO particle to collide
    const leoDebris = SIM_STATE.debrisList.filter(d => d.regime === 'leo' && d.visible);
    if (leoDebris.length > 0) {
      const parent = leoDebris[Math.floor(Math.random() * leoDebris.length)];
      impactPoint = parent.position.clone().multiplyScalar(SCALE_FACTOR);
      baseVelocity = parent.velocity;
    }
  }

  // Print danger notifications in the terminal log stream
  logConsoleMessage(`KESSLER CHAIN REACTION DETECTED`, 'danger');
  logConsoleMessage(`IMPACT POINT: x=${impactPoint.x.toFixed(3)}, y=${impactPoint.y.toFixed(3)}, z=${impactPoint.z.toFixed(3)}`, 'warning');
  logConsoleMessage(`HYPERVELOCITY COLLISION GENERATING FRAGMENTATION CLOUD`, 'warning');

  // Spawn 300-400 fragmentation particles from collision coordinates
  const newFragmentCount = 350;
  const countries = ['us', 'ru', 'cn', 'esa', 'other'];

  // Convert visual position back to orbital physics coordinates (km)
  const physImpactPos = impactPoint.clone().multiplyScalar(EARTH_RADIUS_KM);
  const rDist = physImpactPos.length();

  for (let idx = 0; idx < newFragmentCount; idx++) {
    // Modify Keplerian orbital elements slightly from parent to create a expanding dispersion cloud
    const deviation = 0.02 + Math.random() * 0.15;
    
    // Set a, e, i, raan, arg, ma based on physical location
    const a = rDist * (1.0 + (Math.random() - 0.5) * deviation);
    const e = Math.random() * 0.15;
    const iRad = Math.acos((Math.random() * 2) - 1) * 0.1 + (Math.random() > 0.5 ? 0.9 : 1.4); // clustered orbital angles
    const raan = Math.random() * Math.PI * 2;
    const arg = Math.random() * Math.PI * 2;
    const ma = Math.random() * Math.PI * 2;
    const n = Math.sqrt(MU / Math.pow(a, 3));
    
    const catId = 50000 + SIM_STATE.debrisList.length;

    SIM_STATE.debrisList.push({
      id: catId,
      name: `FRG-${catId}`,
      a,
      e,
      i: iRad,
      raan,
      arg,
      ma,
      n,
      regime: a < 8000 ? 'leo' : a < 25000 ? 'meo' : 'geo',
      category: 'frag',
      country: countries[Math.floor(Math.random() * countries.length)],
      size: 0.05 + Math.random() * 0.4,
      visible: true,
      position: physImpactPos.clone(),
      velocity: baseVelocity + (Math.random() - 0.5) * 1.5,
      altitude: rDist - EARTH_RADIUS_KM,
      risk: 0
    });
  }

  // Re-generate visual particles buffers to fit the new size
  scene.remove(debrisParticles);
  debrisGeometry.dispose();

  const count = SIM_STATE.debrisList.length;
  debrisGeometry = new THREE.BufferGeometry();
  const positions = new Float32Array(count * 3);
  const colors = new Float32Array(count * 3);
  const sizes = new Float32Array(count);

  debrisGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  debrisGeometry.setAttribute('color', new THREE.BufferAttribute(colors, 3));
  debrisGeometry.setAttribute('size', new THREE.BufferAttribute(sizes, 3));

  const particleTex = createParticleTexture();
  const debrisMaterial = new THREE.PointsMaterial({
    size: 0.045,
    vertexColors: true,
    map: particleTex,
    transparent: true,
    opacity: 0.9,
    blending: THREE.AdditiveBlending,
    depthWrite: false
  });

  debrisParticles = new THREE.Points(debrisGeometry, debrisMaterial);
  scene.add(debrisParticles);

  logConsoleMessage(`SPAWNED ${newFragmentCount} NEW FRAGMENT CHIPS IN LOW EARTH ORBIT`, 'info');
}

// Log messages inside scrolling HUD logger
function logConsoleMessage(msg, type = 'normal') {
  const terminal = document.getElementById('log-terminal-stream');
  const entry = document.createElement('div');
  entry.className = 'log-entry';

  // Format timestamp [HH:MM:SS] relative to run
  const now = new Date();
  const timeStr = `[${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}]`;

  entry.innerHTML = `
    <span class="log-time">${timeStr}</span>
    <span class="log-msg ${type}">${msg}</span>
  `;

  terminal.appendChild(entry);
  terminal.scrollTop = terminal.scrollHeight;
}

// Connect HUD GUI components
function initHUD() {
  // Time acceleration slider
  const timeScaleSlider = document.getElementById('time-scale-slider');
  const timeScaleVal = document.getElementById('time-scale-val');
  timeScaleSlider.addEventListener('input', (e) => {
    SIM_STATE.timeScale = parseInt(e.target.value);
    timeScaleVal.innerText = `${SIM_STATE.timeScale}x`;
  });

  // Toggle density volume grid
  const toggleDensityBtn = document.getElementById('toggle-density-grid');
  toggleDensityBtn.addEventListener('click', () => {
    SIM_STATE.showDensityGrid = !SIM_STATE.showDensityGrid;
    toggleDensityBtn.classList.toggle('active', SIM_STATE.showDensityGrid);
    initDensityGrid();
    logConsoleMessage(`Volumetric density grid visibility set to ${SIM_STATE.showDensityGrid ? 'ACTIVE' : 'INACTIVE'}`);
  });

  // Toggle orbit trace lines
  const toggleOrbitsBtn = document.getElementById('toggle-orbit-lines');
  toggleOrbitsBtn.addEventListener('click', () => {
    SIM_STATE.showOrbits = !SIM_STATE.showOrbits;
    toggleOrbitsBtn.classList.toggle('active', SIM_STATE.showOrbits);
    if (!SIM_STATE.showOrbits) {
      clearOrbitLine();
    } else if (SIM_STATE.selectedObject) {
      drawOrbitLine(SIM_STATE.selectedObject);
    }
  });

  // Grid resolution select
  const gridResSelect = document.getElementById('grid-resolution-select');
  gridResSelect.addEventListener('change', (e) => {
    SIM_STATE.gridResolution = parseInt(e.target.value);
    initDensityGrid();
    logConsoleMessage(`Voxel grid resolution updated to ${SIM_STATE.gridResolution}^3`);
  });

  // Filters setup
  const regimeButtons = document.querySelectorAll('#regime-filters button');
  regimeButtons.forEach(btn => {
    btn.addEventListener('click', () => {
      regimeButtons.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      SIM_STATE.filters.regime = btn.getAttribute('data-filter');
      logConsoleMessage(`Regime filter set to: ${SIM_STATE.filters.regime.toUpperCase()}`);
    });
  });

  const categoryButtons = document.querySelectorAll('#category-filters button');
  categoryButtons.forEach(btn => {
    btn.addEventListener('click', () => {
      categoryButtons.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      SIM_STATE.filters.category = btn.getAttribute('data-filter');
      logConsoleMessage(`Category filter set to: ${SIM_STATE.filters.category.toUpperCase()}`);
    });
  });

  // Country select
  const countrySelect = document.getElementById('country-filter-select');
  countrySelect.addEventListener('change', (e) => {
    SIM_STATE.filters.country = e.target.value;
    logConsoleMessage(`Country filter set to: ${SIM_STATE.filters.country.toUpperCase()}`);
  });

  // Kessler simulator action
  const kesslerBtn = document.getElementById('trigger-kessler-btn');
  kesslerBtn.addEventListener('click', triggerKesslerCascade);

  // Initialize density voxel grid Points cloud
  initDensityGrid();
}

// Update selected target details in sidebar card
function updateSelectedObjectCard() {
  const cardName = document.getElementById('card-obj-name');
  const cardId = document.getElementById('card-obj-id');
  const cardAlt = document.getElementById('card-obj-alt');
  const cardVel = document.getElementById('card-obj-vel');
  const cardInc = document.getElementById('card-obj-inc-ecc');
  const cardRisk = document.getElementById('card-obj-risk');

  if (SIM_STATE.selectedObject) {
    const deb = SIM_STATE.selectedObject;
    cardName.innerText = deb.name;
    cardId.innerText = deb.id;
    cardAlt.innerText = `${Math.round(deb.altitude)} km`;
    cardVel.innerText = `${deb.velocity.toFixed(2)} km/s`;
    
    const incDeg = Math.round(deb.i * (180 / Math.PI));
    cardInc.innerText = `${incDeg}° / e=${deb.e.toFixed(3)}`;
    
    // Risk coloring based on rating
    cardRisk.innerText = `${deb.risk.toFixed(1)}%`;
    if (deb.risk > 70) {
      cardRisk.className = 'info-value red';
    } else if (deb.risk > 30) {
      cardRisk.className = 'info-value amber';
    } else {
      cardRisk.className = 'info-value emerald';
    }
  } else {
    cardName.innerText = 'Select an object...';
    cardId.innerText = 'N/A';
    cardAlt.innerText = 'N/A';
    cardVel.innerText = 'N/A';
    cardInc.innerText = 'N/A';
    cardRisk.innerText = 'N/A';
    cardRisk.className = 'info-value';
  }
}

// Handle window resizing
function onWindowResize() {
  const container = document.getElementById('canvas-container');
  camera.aspect = container.clientWidth / container.clientHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(container.clientWidth, container.clientHeight);
}

// Main Frame Rendering Loop
function animate(currentTime) {
  requestAnimationFrame(animate);

  const delta = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;

  // Increment simulation clock
  SIM_STATE.simTime += delta * SIM_STATE.timeScale;
  document.getElementById('sim-time-val').innerText = `${SIM_STATE.simTime.toFixed(1)}s`;

  // Orbit Earth slowly
  if (earthMesh) {
    earthMesh.rotation.y += 0.015 * delta;
  }

  // Directional sun rotation to update shadows
  if (sunLight) {
    const angle = SIM_STATE.simTime * 0.00005;
    sunLight.position.set(5 * Math.cos(angle), 3, 5 * Math.sin(angle));
  }

  // Update positions and visual meshes
  updateDebrisParticles();
  updateDensityGrid();
  updateSelectedObjectCard();
  updateAnalyticsCharts();

  // Damping controls update
  controls.update();

  renderer.render(scene, camera);
}
