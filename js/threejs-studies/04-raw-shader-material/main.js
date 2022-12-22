// From https://threejs.org/docs/#api/en/materials/PointsMaterial
import * as THREE from 'three';
import Stats from 'three/addons/libs/stats.module.js';
import { GUI } from 'three/addons/libs/lil-gui.module.min.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

const params = {
  shaderSpeed: 1.0,
  boxSpeed: 1.0,
};

const container = document.getElementById('container');
const stats = new Stats();
container.appendChild(stats.dom);
const gui = new GUI({ title: 'settings' });
gui.add(params, 'shaderSpeed', 0, 5);
gui.add(params, 'boxSpeed', 0, 5);

const renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 1, 1000);
camera.position.set(3, 3, 10);
camera.lookAt(0, 0, 0);
const controls = new OrbitControls(camera, container);
controls.listenToKeyEvents(window); // optional
controls.enableDamping = true; // an animation loop is required when either damping or auto-rotation are enabled
controls.dampingFactor = 0.05;
controls.screenSpacePanning = false;

const geometry = new THREE.BoxGeometry(1, 2, 3, 1, 1, 1);
geometry.position = new THREE.Vector3(0, 0, 0);

// const material = new THREE.MeshBasicMaterial({ color: 0x888888 });
const material = new THREE.RawShaderMaterial({
  uniforms: {
    time: { value: 1.0 }
  },
  vertexShader: document.getElementById('vertexShader').textContent,
  fragmentShader: document.getElementById('fragmentShader').textContent,
  side: THREE.FrontSide,
  transparent: false
});

const object = new THREE.Mesh(geometry, material);

const scene = new THREE.Scene();
scene.add(object);

window.addEventListener('resize', onWindowResize);

function animate() {
  requestAnimationFrame(animate);
  controls.update(); // only required if controls.enableDamping = true, or if controls.autoRotate = true

  const time = performance.now() / 1000; // sec
  object.position.y = Math.sin(2 * Math.PI * time * params.boxSpeed) * 0.5;
  object.material.uniforms.time.value = time * params.shaderSpeed;

  renderer.render(scene, camera);
  stats.update();
}

function onWindowResize() {
  camera.aspect = container.offsetWidth / container.offsetHeight;
  camera.updateProjectionMatrix();

  renderer.setSize(container.offsetWidth, container.offsetHeight);
}

animate();