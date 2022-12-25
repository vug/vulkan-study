// From https://threejs.org/docs/#api/en/materials/PointsMaterial
import * as THREE from 'three';
import Stats from 'three/addons/libs/stats.module.js';
import { GUI } from 'three/addons/libs/lil-gui.module.min.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

const settings = {
  useThreeJsLightingModel: false,
};

const container = document.getElementById('container');
const stats = new Stats();
container.appendChild(stats.dom);
const gui = new GUI({ title: 'Settings' });
{
  const folder = gui.addFolder('Main');
  folder.add(settings, 'useThreeJsLightingModel');
}

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 100);
camera.position.set(0, 10, 20);
camera.lookAt(0, 0, 0);
const controls = new OrbitControls(camera, container);
controls.listenToKeyEvents(window); // optional
controls.target.set(0, 5, 0);
controls.enableDamping = true; // an animation loop is required when either damping or auto-rotation are enabled
controls.dampingFactor = 0.05;
controls.screenSpacePanning = false;

const scene = new THREE.Scene();
const textureLoader = new THREE.TextureLoader();

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
  const planeSize = 40;
  const checkerTex = textureLoader.load('/assets/images/checker.png');
  checkerTex.wrapS = THREE.RepeatWrapping;
  checkerTex.wrapT = THREE.RepeatWrapping;
  checkerTex.magFilter = THREE.NearestFilter;
  const repeats = planeSize / 2;
  checkerTex.repeat.set(repeats, repeats);

  const planeGeo = new THREE.PlaneGeometry(planeSize, planeSize);
  const planeMat = new THREE.MeshPhongMaterial({
    map: checkerTex,
    side: THREE.DoubleSide,
  });
  const planeMesh = new THREE.Mesh(planeGeo, planeMat);
  planeMesh.rotation.x = -0.5 * Math.PI;
  scene.add(planeMesh);
}


let addObject = (geo, mat, pos) => {
  const obj = new THREE.Mesh(geo, mat);
  obj.position.set(pos.x, pos.y, pos.z);
  scene.add(obj);
  return obj;
};

const geometry1 = new THREE.BoxGeometry(1, 2, 3, 1, 1, 1);
const material1 = new THREE.MeshPhongMaterial({ color: '#CA8' });
const object1 = addObject(geometry1, material1, new THREE.Vector3(3, 2, 0));

const geometry2 = new THREE.BoxGeometry(1, 2, 3, 1, 1, 1);
const material2 = new THREE.ShaderMaterial({
  uniforms: {
    ...THREE.UniformsUtils.clone(THREE.UniformsLib.lights),
    time: { value: 1.0 },
    color: { value: new THREE.Vector3(204. / 255, 136. / 255, 170. / 255) },
    useThreeJsLightingModel: { value: settings.useThreeJsLightingModel },
  },
  vertexShader: document.getElementById('vertexShader').textContent,
  fragmentShader: document.getElementById('fragmentShader').textContent,
  lights: true,
  glslVersion: THREE.GLSL3,
});
const object2 = addObject(geometry2, material2, new THREE.Vector3(0, 2, 0));
console.log(object2.material.uniforms);

const geometry3 = new THREE.SphereGeometry(1, 32, 16);
const material3 = new THREE.MeshPhongMaterial({ color: '#8AC' });
const object3 = addObject(geometry3, material3, new THREE.Vector3(-3, 2, 0));

window.addEventListener('resize', onWindowResize);

function animate() {
  requestAnimationFrame(animate);
  controls.update(); // only required if controls.enableDamping = true, or if controls.autoRotate = true
  object2.material.uniforms.useThreeJsLightingModel.value = settings.useThreeJsLightingModel;

  const time = performance.now() / 1000; // sec

  renderer.render(scene, camera);
  stats.update();
}

function onWindowResize() {
  camera.aspect = container.offsetWidth / container.offsetHeight;
  camera.updateProjectionMatrix();

  renderer.setSize(container.offsetWidth, container.offsetHeight);
}

animate();