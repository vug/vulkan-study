// From https://threejs.org/docs/#api/en/materials/PointsMaterial
import * as THREE from 'three';
import Stats from 'three/addons/libs/stats.module.js';
import { GUI } from 'three/addons/libs/lil-gui.module.min.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

const visualizationOptions = { scene: 0, depthOverrideMaterial: 1, depthPostProcessUgur: 2, depthPostProcessLearnOpenGL: 3 };
const settings = {
  visualize: visualizationOptions.scene,
  cameraFar: 50.0,
};

let container, stats, gui;
let renderer, controls, scene;
/** @type {THREE.PerspectiveCamera} */
let camera;
let depthMaterial = new THREE.MeshDepthMaterial();
let renderTarget;
let postScene, postCamera;
/** @type {THREE.ShaderMaterial} */
let postMaterial;
let supportsExtension = true;
// const textureLoader = new THREE.TextureLoader();


function init() {
  container = document.getElementById('container');

  stats = new Stats();
  container.appendChild(stats.dom);

  gui = new GUI({ title: 'Settings' });
  {
    const folder = gui.addFolder('Main');
    folder.add(settings, 'visualize', visualizationOptions);
    folder.add(settings, 'cameraFar', 0.1, 100);
  }

  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(window.innerWidth, window.innerHeight);
  document.body.appendChild(renderer.domElement);

  if (!renderer.capabilities.isWebGL2 && !renderer.extensions.has('WEBGL_depth_texture')) {
    supportsExtension = false;
    console.warn("Depth Texture extension is not supported. Exiting early...");
    return;
  }

  camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, settings.cameraFar);
  camera.position.set(0, 10, 20);
  camera.lookAt(0, 0, 0);
  controls = new OrbitControls(camera, container);
  controls.listenToKeyEvents(window);
  controls.target.set(0, 0, 0);
  controls.enableDamping = true; // an animation loop is required when either damping or auto-rotation are enabled
  controls.dampingFactor = 0.05;
  controls.screenSpacePanning = false;

  // Setup renderTarget
  if (renderTarget) renderTarget.dispose();
  let size = new THREE.Vector2();
  renderer.getSize(size);
  renderTarget = new THREE.WebGLRenderTarget(size.x, size.y);
  renderTarget.texture.minFilter = THREE.NearestFilter;
  renderTarget.texture.magFilter = THREE.NearestFilter;
  renderTarget.stencilBuffer = true;
  renderTarget.depthTexture = new THREE.DepthTexture();
  renderTarget.depthTexture.format = THREE.DepthStencilFormat;
  renderTarget.depthTexture.type = THREE.UnsignedInt248Type;

  // Setup Scene
  scene = new THREE.Scene();
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

  // Setup Post-Processing Stage
  postCamera = new THREE.OrthographicCamera(-1, 1, 1, -1, 0, 1);
  postMaterial = new THREE.ShaderMaterial({
    vertexShader: document.querySelector('#post-vert').textContent.trim(),
    fragmentShader: document.querySelector('#post-frag').textContent.trim(),
    uniforms: {
      cam: {
        value: {
          near: camera.near,
          far: camera.far,
        }
      },
      tDiffuse: { value: null },
      tDepth: { value: null },
      linearizationMethod: { value: settings.visualize },
    }
  });
  const postPlane = new THREE.PlaneGeometry(2, 2);
  const postQuad = new THREE.Mesh(postPlane, postMaterial);
  postScene = new THREE.Scene();
  postScene.add(postQuad);

  // debugger;
}

function animate() {
  if (!supportsExtension) return;

  requestAnimationFrame(animate);
  controls.update(); // only required if controls.enableDamping = true, or if controls.autoRotate = true

  camera.far = settings.cameraFar;
  postMaterial.uniforms.cam.value.far = camera.far;

  const time = performance.now() / 1000; // sec

  if (settings.visualize === visualizationOptions.scene) {
    renderer.render(scene, camera);
  }
  else if (settings.visualize === visualizationOptions.depthOverrideMaterial) {
    // scene.overrideMaterial = new THREE.MeshDistanceMaterial();
    scene.overrideMaterial = depthMaterial;
    renderer.render(scene, camera);
    scene.overrideMaterial = null;
  }
  else if (settings.visualize === visualizationOptions.depthPostProcessUgur || settings.visualize === visualizationOptions.depthPostProcessLearnOpenGL) {
    postMaterial.uniforms.linearizationMethod.value = settings.visualize;
    // Render scene as usual into renderTarget
    renderer.setRenderTarget(renderTarget);
    renderer.render(scene, camera);

    // Render post-processing
    postMaterial.uniforms.tDiffuse.value = renderTarget.texture;
    postMaterial.uniforms.tDepth.value = renderTarget.depthTexture;
    renderer.setRenderTarget(null);
    renderer.render(postScene, postCamera);
  }

  stats.update();
}

window.addEventListener('resize', onWindowResize);
init();
animate();

function onWindowResize() {
  const width = container.offsetWidth;
  const height = container.offsetHeight;
  camera.aspect = width / height;
  camera.updateProjectionMatrix();

  renderer.setSize(width, height);

  const dpr = renderer.getPixelRatio();
  renderTarget.setSize(width * dpr, height * dpr);
}
