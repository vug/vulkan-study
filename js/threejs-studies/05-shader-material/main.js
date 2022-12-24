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

const color = 0xFFFFFF;
const intensity = 1;
const light = new THREE.AmbientLight(color, intensity);
scene.add(light);

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
const material1 = new THREE.RawShaderMaterial({
  uniforms: {
    time: { value: 1.0 }
  },
  vertexShader: document.getElementById('vertexShader').textContent,
  fragmentShader: document.getElementById('fragmentShader1').textContent,
  glslVersion: THREE.GLSL3,
});
const object1 = addObject(geometry1, material1, new THREE.Vector3(3, 0, 0));

const geometry2 = new THREE.SphereGeometry(1, 32, 16);
const material2 = new THREE.RawShaderMaterial({
  vertexShader: document.getElementById('vertexShader').textContent,
  fragmentShader: document.getElementById('fragmentShader2').textContent,
  glslVersion: THREE.GLSL3,
});
const object2 = addObject(geometry2, material2, new THREE.Vector3(0, 0, 0));

const geometry3 = new THREE.BufferGeometry();
{
  const vertices = [
    -0.5, 0.5, 0.0,
    -0.5, -0.5, 0.0,
    0.5, -0.5, 0.0,
    0.5, 0.5, 0.0,
  ];
  const uv = [
    0, 1,
    0, 0,
    1, 0,
    1, 1,
  ];
  const colors = [
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
  ];
  const indicies = [
    0, 1, 2,
    0, 2, 3,
  ];
  geometry3.setIndex(indicies);
  geometry3.setAttribute('position', new THREE.Float32BufferAttribute(vertices, 3));
  geometry3.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
  let colorAttr = geometry3.setAttribute('color', new THREE.Float32BufferAttribute(colors, 4));
  colorAttr.normalized = true;
}
const material3 = new THREE.RawShaderMaterial({
  vertexShader: document.getElementById('vertexShader').textContent,
  fragmentShader: document.getElementById('fragmentShader3').textContent,
  uniforms: {
    time: { value: 1.0 },
    tAlbedo1: { value: textureLoader.load('/assets/images/uv_grid_opengl.jpg') },
    tAlbedo2: { value: textureLoader.load('/assets/images/crate.gif') },
  },
  glslVersion: THREE.GLSL3,
});
const object3 = addObject(geometry3, material3, new THREE.Vector3(-3, 0, 0));


// const material = new THREE.MeshBasicMaterial({ color: 0x888888 });

window.addEventListener('resize', onWindowResize);

function animate() {
  requestAnimationFrame(animate);
  controls.update(); // only required if controls.enableDamping = true, or if controls.autoRotate = true

  const time = performance.now() / 1000; // sec
  object1.material.uniforms.time.value = time * params.shaderSpeed;
  object1.setRotationFromAxisAngle(new THREE.Vector3(0, 1, 0), time * params.boxSpeed);

  object2.position.y = Math.sin(2 * Math.PI * time * params.boxSpeed) * 0.5;

  object3.material.uniforms.time.value = time * params.shaderSpeed;
  const sz = Math.sin(time) + 1.5;
  object3.scale.set(sz, sz, sz);


  renderer.render(scene, camera);
  stats.update();
}

function onWindowResize() {
  camera.aspect = container.offsetWidth / container.offsetHeight;
  camera.updateProjectionMatrix();

  renderer.setSize(container.offsetWidth, container.offsetHeight);
}

animate();