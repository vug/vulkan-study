// From https://threejs.org/docs/#api/en/materials/PointsMaterial
import * as THREE from 'three';
import Stats from 'three/addons/libs/stats.module.js';
import { GUI } from 'three/addons/libs/lil-gui.module.min.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

const visualizationOptions = { sceneColor: 0, sceneDepth: 1, depthUgur: 2, outline: 3 };
const settings = {
  visualize: visualizationOptions.outline,
  cameraFar: 50.0,
  param: 0.5,
};


// INIT at global so that every function that uses shared variables have type hints
// const textureLoader = new THREE.TextureLoader();
const container = document.getElementById('container');

const stats = new Stats();
container.appendChild(stats.dom);

const gui = new GUI({ title: 'Settings' });
{
  const folder = gui.addFolder('Main');
  folder.add(settings, 'visualize', visualizationOptions);
  folder.add(settings, 'cameraFar', 0.1, 100);
  folder.add(settings, 'param', 0.0, 1.0);
}

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);
if (!renderer.capabilities.isWebGL2 && !renderer.extensions.has('WEBGL_depth_texture')) {
  console.warn("Depth Texture extension is not supported. Exiting early...");
  debugger;
}
const devicePixelRatio = renderer.getPixelRatio();

const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, settings.cameraFar);
camera.position.set(0, 10, 20);
camera.lookAt(0, 0, 0);
const controls = new OrbitControls(camera, container);
controls.listenToKeyEvents(window);
controls.target.set(0, 0, 0);
controls.enableDamping = true; // an animation loop is required when either damping or auto-rotation are enabled
controls.dampingFactor = 0.05;
controls.screenSpacePanning = false;

// Pass1: scene with depth -> rtSceneWDepth
// Pass2: outline via tSceneDepth -> rtOutline
// Pass3: compose tSceneColor + tOutline -> screen

// Setup renderTargets
const targetSize = new THREE.Vector2();
renderer.getSize(targetSize);
targetSize.multiplyScalar(devicePixelRatio);

const renderTargetSceneWithDepth = new THREE.WebGLRenderTarget(targetSize.x, targetSize.y);
renderTargetSceneWithDepth.texture.minFilter = THREE.NearestFilter;
renderTargetSceneWithDepth.texture.magFilter = THREE.NearestFilter;
renderTargetSceneWithDepth.stencilBuffer = true;
renderTargetSceneWithDepth.depthTexture = new THREE.DepthTexture();
renderTargetSceneWithDepth.depthTexture.format = THREE.DepthStencilFormat;
renderTargetSceneWithDepth.depthTexture.type = THREE.UnsignedInt248Type;

const renderTargetOutline = new THREE.WebGLMultipleRenderTargets(targetSize.x, targetSize.y, 2);
renderTargetOutline.texture[0].name = 'Outline';
renderTargetOutline.texture[1].name = 'Depth';
for (let i = 0, il = renderTargetOutline.texture.length; i < il; i++) {
  renderTargetOutline.texture[i].minFilter = THREE.NearestFilter;
  renderTargetOutline.texture[i].magFilter = THREE.NearestFilter;
}

// Setup Scene
const scene = new THREE.Scene();
{
  const light = new THREE.AmbientLight(0xFFFFFF, 0.1);
  scene.add(light);

  const folder = gui.addFolder('Ambient Light');
  folder.addColor(light, 'color');
  folder.add(light, 'intensity', 0, 2, 0.01)
}
{
  const light = new THREE.HemisphereLight(0xB1E1FF, 0xB97A20, 0.25);
  scene.add(light);

  const folder = gui.addFolder('Hemisphere Light');
  folder.addColor(light, 'color').name('skyColor');
  folder.addColor(light, 'groundColor');
  folder.add(light, 'intensity', 0, 2, 0.01)
}
{
  const geometry = new THREE.TorusKnotGeometry(1, 0.3, 128, 64);
  // const material = new THREE.MeshBasicMaterial({ color: 'blue' });
  const material = new THREE.MeshStandardMaterial({
    color: 0xAABBCC,
    metalness: 0.5,
    roughness: 0.6,
  });
  const count = 50;
  const scale = 5;
  for (let i = 0; i < count; i++) {
    const r = Math.random() * 2.0 * Math.PI;
    const z = (Math.random() * 2.0) - 1.0;
    const zScale = Math.sqrt(1.0 - z * z) * scale;
    const mesh = new THREE.Mesh(geometry, material);
    mesh.position.set(
      Math.cos(r) * zScale,
      Math.sin(r) * zScale,
      z * scale
    );
    mesh.rotation.set(Math.random(), Math.random(), Math.random());
    scene.add(mesh);
  }
}

// Post-Processing Objects
const postCamera = new THREE.OrthographicCamera(-1, 1, 1, -1, 0, 1);
const postPlane = new THREE.PlaneGeometry(2, 2);

// Outline Pass
const outlineMaterial = new THREE.RawShaderMaterial({
  vertexShader: document.querySelector('#post-vert-raw').textContent.trim(),
  fragmentShader: document.querySelector('#post-outline-raw').textContent.trim(),
  uniforms: {
    cam: {
      value: {
        near: camera.near,
        far: camera.far,
      }
    },
    tDiffuse: { value: null },
    tDepth: { value: null },
    param: { value: settings.param },
  },
  glslVersion: THREE.GLSL3,
});
const outlineQuad = new THREE.Mesh(postPlane, outlineMaterial);
const outlineScene = new THREE.Scene();
outlineScene.add(outlineQuad);

// Final Pass
const finalMaterial = new THREE.ShaderMaterial({
  vertexShader: document.querySelector('#post-vert').textContent.trim(),
  fragmentShader: document.querySelector('#post-screen').textContent.trim(),
  uniforms: {
    tTexture: { value: null },
  }
});
const finalQuad = new THREE.Mesh(postPlane, finalMaterial);
const finalScene = new THREE.Scene();
finalScene.add(finalQuad);

function animate() {
  requestAnimationFrame(animate);
  controls.update(); // only required if controls.enableDamping = true, or if controls.autoRotate = true

  camera.far = settings.cameraFar;
  outlineMaterial.uniforms.cam.value.far = camera.far;
  outlineMaterial.uniforms.param.value = settings.param;

  const time = performance.now() / 1000; // sec

  // Main Scene Pass with Depth
  renderer.setRenderTarget(renderTargetSceneWithDepth);
  renderer.render(scene, camera);

  // Outline Pass
  outlineMaterial.uniforms.tDiffuse.value = renderTargetSceneWithDepth.texture;
  outlineMaterial.uniforms.tDepth.value = renderTargetSceneWithDepth.depthTexture;
  renderer.setRenderTarget(renderTargetOutline);
  renderer.render(outlineScene, postCamera);

  // Final Screen Pass
  const textures = [
    renderTargetSceneWithDepth.texture,
    renderTargetSceneWithDepth.depthTexture,
    renderTargetOutline.texture[1],
    renderTargetOutline.texture[0],
  ];
  finalMaterial.uniforms.tTexture.value = textures[settings.visualize];
  renderer.setRenderTarget(null);
  renderer.render(finalScene, postCamera);

  stats.update();
}

window.addEventListener('resize', onWindowResize);
animate();

function onWindowResize() {
  const width = container.offsetWidth;
  const height = container.offsetHeight;
  camera.aspect = width / height;
  camera.updateProjectionMatrix();

  renderer.setSize(width, height);

  renderTargetSceneWithDepth.setSize(targetSize.x, targetSize.y);
  renderTargetOutline.setSize(targetSize.x, targetSize.y);
}
