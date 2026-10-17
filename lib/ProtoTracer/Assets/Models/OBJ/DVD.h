#pragma once

#include "../../../Scene/Materials/Static/SimpleMaterial.h"
#include "../../../Scene/Objects/Object3D.h"

class DVD{
private:
	Vector3D basisVertices[42] = {Vector3D(6.2089f,68.0156f,0.0000f),Vector3D(4.7978f,62.3711f,0.0000f),Vector3D(22.3035f,62.2405f,0.0000f),Vector3D(43.1800f,68.0156f,0.0000f),Vector3D(35.8422f,62.3711f,0.0000f),Vector3D(24.5533f,58.4859f,0.0000f),Vector3D(27.4284f,44.2819f,0.0000f),Vector3D(12.0129f,48.2600f,0.0000f),Vector3D(0.2822f,42.6156f,0.0000f),Vector3D(4.7978f,59.8311f,0.0000f),Vector3D(14.1111f,59.8311f,0.0000f),Vector3D(44.2680f,37.6562f,0.0000f),Vector3D(48.5422f,51.3644f,0.0000f),Vector3D(47.2478f,37.6971f,0.0000f),Vector3D(66.6044f,62.3711f,0.0000f),Vector3D(60.6778f,68.0156f,0.0000f),Vector3D(84.6667f,68.0156f,0.0000f),Vector3D(83.9427f,62.3691f,0.0000f),Vector3D(96.2378f,62.9356f,0.0000f),Vector3D(88.0533f,58.1378f,0.0000f),Vector3D(89.2350f,43.9997f,0.0000f),Vector3D(73.3778f,48.2600f,0.0000f),Vector3D(62.3711f,42.6156f,0.0000f),Vector3D(66.8867f,59.8311f,0.0000f),Vector3D(75.9178f,59.8311f,0.0000f),Vector3D(0.0000f,30.7622f,0.0000f),Vector3D(4.7978f,27.0933f,0.0000f),Vector3D(8.6552f,35.1920f,0.0000f),Vector3D(19.8314f,24.0298f,0.0000f),Vector3D(22.2956f,36.6889f,0.0000f),Vector3D(37.8178f,22.8600f,0.0000f),Vector3D(34.3982f,31.0425f,0.0000f),Vector3D(46.7651f,32.8195f,0.0000f),Vector3D(58.0231f,31.0444f,0.0000f),Vector3D(46.3432f,28.5842f,0.0000f),Vector3D(53.3400f,22.8600f,0.0000f),Vector3D(55.3156f,37.8178f,0.0000f),Vector3D(72.5311f,36.9711f,0.0000f),Vector3D(89.2680f,34.2287f,0.0000f),Vector3D(96.4033f,30.2367f,0.0000f),Vector3D(89.2422f,26.3225f,0.0000f),Vector3D(71.9667f,23.4244f,0.0000f)};
	IndexGroup basisIndexes[41] = {IndexGroup(1,2,0),IndexGroup(2,3,0),IndexGroup(4,3,2),IndexGroup(5,4,2),IndexGroup(5,6,4),IndexGroup(7,6,5),IndexGroup(7,8,6),IndexGroup(7,9,8),IndexGroup(9,7,10),IndexGroup(11,12,4),IndexGroup(3,4,12),IndexGroup(13,12,11),IndexGroup(14,12,13),IndexGroup(14,15,12),IndexGroup(14,16,15),IndexGroup(17,16,14),IndexGroup(17,18,16),IndexGroup(19,18,17),IndexGroup(19,20,18),IndexGroup(21,20,19),IndexGroup(21,22,20),IndexGroup(23,22,21),IndexGroup(24,23,21),IndexGroup(26,27,25),IndexGroup(26,28,27),IndexGroup(28,29,27),IndexGroup(30,31,28),IndexGroup(29,28,31),IndexGroup(31,11,29),IndexGroup(32,11,31),IndexGroup(13,11,32),IndexGroup(33,13,32),IndexGroup(34,31,30),IndexGroup(35,34,30),IndexGroup(36,13,33),IndexGroup(33,37,36),IndexGroup(33,38,37),IndexGroup(39,38,33),IndexGroup(39,33,40),IndexGroup(40,33,41),IndexGroup(41,34,35)};
	StaticTriangleGroup<42,41> triangleGroup = StaticTriangleGroup<42,41>(&basisVertices[0], &basisIndexes[0]);
	TriangleGroup<42,41> triangleGroupMemory = TriangleGroup<42,41>(&triangleGroup);
	SimpleMaterial simpleMaterial = SimpleMaterial(RGBColor(128, 128, 128));
	Object3D basisObj = Object3D(&triangleGroup, &triangleGroupMemory, &simpleMaterial);

public:
	DVD(){}

	Object3D* GetObject(){
		return &basisObj;
	}
};
