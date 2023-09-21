//Maya ASCII 2023 scene
//Name: multi_uv_simple.ma
//Last modified: Wed, Oct 11, 2023 06:50:06 PM
//Codeset: UTF-8
requires maya "2022";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2023";
fileInfo "version" "2022";
fileInfo "cutIdentifier" "202208031415-1dee56799d";
fileInfo "osv" "Mac OS X 10.16";
fileInfo "UUID" "2C73540B-4A47-B5D3-ADF2-BC84A68979B6";
createNode transform -n "multi_uv";
	rename -uid "1809FCF5-A942-F129-00A2-3595960693D3";
createNode transform -n "mesh_template24x" -p "multi_uv";
	rename -uid "FEBE8ECB-0245-3DFC-A3B0-2D9B1ADDA032";
	setAttr ".hdl" -type "double3" 141.6311137495947 0 -2 ;
	setAttr ".dh" yes;
createNode mesh -n "mesh_template24xShape" -p "mesh_template24x";
	rename -uid "91F7C3AB-4141-6C61-E3FA-AD917BF85BAA";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.5 0.5 ;
	setAttr -s 2 ".uvst";
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 4 ".uvst[0].uvsp[0:3]" -type "float2" 0 0 1 0 0 1 1 1;
	setAttr ".uvst[1].uvsn" -type "string" "map2";
	setAttr -s 4 ".uvst[1].uvsp[0:3]" -type "float2" -0.91421354 0.5 0.5
		 -0.91421354 1.91421354 0.5 0.5 1.91421354;
	setAttr ".cuvs" -type "string" "map2";
	setAttr ".dcol" yes;
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 4 ".pt[0:3]" -type "float3"  10.000008 -17.346029 3.5527137e-15 
		10.000008 -17.346029 3.5527137e-15 10.000008 -17.346029 3.5527137e-15 10.000008 -17.346029 
		3.5527137e-15;
	setAttr -s 4 ".vt[0:3]"  -15 12.34603024 -3.5527137e-15 -5 12.34603024 -3.5527137e-15
		 -15 22.34603119 -3.5527137e-15 -5 22.34603119 -3.5527137e-15;
	setAttr -s 4 ".ed[0:3]"  0 1 0 0 2 0 1 3 0 2 3 0;
	setAttr -ch 4 ".fc[0]" -type "polyFaces" 
		f 4 0 2 -4 -2
		mu 0 4 0 1 3 2
		mu 1 4 0 1 2 3;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr -s 2 ".pd";
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".pd[1]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".dmb" yes;
	setAttr ".de" 1;
createNode transform -n "label_utility_linear_srgb4" -p "multi_uv";
	rename -uid "998CA060-7148-02EA-326F-1AA4B79D95EA";
	setAttr ".rp" -type "double3" -4.9999923706054688 -6.4689668792141788 0 ;
	setAttr ".sp" -type "double3" -4.9999923706054688 -6.4689668792141788 0 ;
createNode mesh -n "label_utility_linear_srgb4Shape" -p "label_utility_linear_srgb4";
	rename -uid "9CDDA7F5-224D-8A85-6B99-9E848300F986";
	setAttr -k off ".v";
	setAttr -s 6 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcol" yes;
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".dmb" yes;
	setAttr ".de" 1;
createNode transform -s -n "persp";
	rename -uid "B9F6A7FB-AF43-A679-43BB-8DADCF9C7F6C";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 3.2865971517835044 7.1138463979318729 32.355946092916113 ;
	setAttr ".r" -type "double3" -12.338352729602548 5.7999999999999803 -9.9903770284925311e-17 ;
createNode camera -s -n "perspShape" -p "persp";
	rename -uid "92B408C6-BD45-6D6D-5916-0182A6C75485";
	setAttr -k off ".v" no;
	setAttr ".fl" 34.999999999999986;
	setAttr ".coi" 33.291376946278056;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".hc" -type "string" "viewSet -p %camera";
createNode transform -s -n "top";
	rename -uid "A65707C6-614C-3639-1C63-1DB8D7D7288D";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 1000.1 0 ;
	setAttr ".r" -type "double3" -89.999999999999986 0 0 ;
createNode camera -s -n "topShape" -p "top";
	rename -uid "1AFA84C0-1941-BD55-E4A9-FA88EE26DCA3";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "top";
	setAttr ".den" -type "string" "top_depth";
	setAttr ".man" -type "string" "top_mask";
	setAttr ".hc" -type "string" "viewSet -t %camera";
	setAttr ".o" yes;
createNode transform -s -n "front";
	rename -uid "9802CEC6-364C-5D0C-6E4E-51AF5AE86B23";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 0 1000.1 ;
createNode camera -s -n "frontShape" -p "front";
	rename -uid "947AD2A9-6E4E-30E6-0D94-6F9D00E789E8";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "front";
	setAttr ".den" -type "string" "front_depth";
	setAttr ".man" -type "string" "front_mask";
	setAttr ".hc" -type "string" "viewSet -f %camera";
	setAttr ".o" yes;
createNode transform -s -n "side";
	rename -uid "857FCB7F-D04D-47B8-0AF6-90A193EBF2B2";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 1000.1 0 0 ;
	setAttr ".r" -type "double3" 0 89.999999999999986 0 ;
createNode camera -s -n "sideShape" -p "side";
	rename -uid "F2DF9414-1540-5188-1A82-61941124853C";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "side";
	setAttr ".den" -type "string" "side_depth";
	setAttr ".man" -type "string" "side_mask";
	setAttr ".hc" -type "string" "viewSet -s %camera";
	setAttr ".o" yes;
createNode materialInfo -n "materialInfo40";
	rename -uid "AAAAE064-2649-F2C2-1AE4-2DB36D0F509A";
createNode shadingEngine -n "multi_uv_set_matSG";
	rename -uid "8D0A2ED7-214A-C2A9-7BC1-C1A4786FD2D6";
	setAttr ".ro" yes;
createNode usdPreviewSurface -n "multi_uv_set_mat";
	rename -uid "53E241DE-9542-2711-69AC-9E90C73F916C";
	setAttr ".rgh" 0;
createNode file -n "texcoord_checker_up_bc";
	rename -uid "6E459667-6B4F-D407-448C-02867411F938";
	setAttr ".ftn" -type "string" "/Volumes/Data/bugs/maya_multi_uv_simple/sourceimages/label_bc.png";
	setAttr ".cs" -type "string" "Input - Texture - sRGB - Display P3";
	setAttr ".ifr" yes;
createNode place2dTexture -n "place2dTexture25";
	rename -uid "0FB663F3-F04B-B1EF-74EB-F9A1FB66CA32";
createNode uvChooser -n "uvChooser1";
	rename -uid "DCCE2F32-564A-9E87-782A-27A70FA55A66";
createNode file -n "uv_grid_ao";
	rename -uid "4232A0AC-DA44-6D89-C881-7384AA789D0C";
	setAttr ".ftn" -type "string" "/Volumes/Data/bugs/maya_multi_uv_simple/sourceimages/label_ao.png";
	setAttr ".cs" -type "string" "Raw";
	setAttr ".ifr" yes;
createNode place2dTexture -n "place2dTexture26";
	rename -uid "BF0B9EF2-414D-B935-C98B-88ABFAA8BF80";
createNode file -n "uv_grid_ao_opacity";
	rename -uid "264ED64E-484B-02CD-2132-2BB8768640DE";
	setAttr ".ftn" -type "string" "/Volumes/Data/bugs/maya_multi_uv_simple/sourceimages/label_o.png";
	setAttr ".cs" -type "string" "Utility - Raw";
	setAttr ".ifr" yes;
createNode place2dTexture -n "place2dTexture41";
	rename -uid "B5D3DC31-3547-7F99-474C-8985FB4AC028";
createNode groupId -n "vectorAdjust1GroupId120";
	rename -uid "3BD0DBBD-784A-9EB2-9B60-BC941E2847B4";
	setAttr ".ihi" 0;
createNode objectSet -n "vectorAdjust1Set120";
	rename -uid "29B6566D-D043-7B5D-2A3E-C082E3770D48";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode vectorAdjust -n "vectorAdjust121";
	rename -uid "6B43B801-9148-3290-5FC8-E1B0C63BEF3D";
	setAttr ".ip[0].gtg" -type "string" "";
	setAttr ".extrudeDistanceScalePP" -type "doubleArray" 0 ;
	setAttr ".boundingBoxes" -type "vectorArray" 40 0.86428505182266235 -0.24903891980648041
		 0 0.86428505182957016 -0.24903891979543388 0 8.7747774124145508 -0.22698548436164856
		 0 8.774777412421269 -0.22698548435334912 0 17.506591796875 0 0 17.506591796876531
		 7.7428603172302244e-12 0 25.146459579467773 0 0 25.146459579478329 8.0211534500122061e-12
		 0 37.276588439941406 -0.26359120011329651 0 37.276588439948824 -0.26359120010499704
		 0 45.883037567138672 -3.1278388500213623 0 45.883037567145578 -3.1278388500101988
		 0 53.830284118652344 0 0 53.830284118659598 1.0526544570922852e-11 0 0.60054385662078857
		 -15.263590812683105 0 0.60054385662820919 -15.263590812674806 0 8.7747774124145508
		 -15.28564453125 0 8.7747774124218623 -15.285644531241642 0 18.348129272460938 -15
		 0 18.348129272462469 -14.999999999992257 0 25.987998962402344 -15 0 25.987998962412899
		 -14.999999999991978 0 38.118125915527344 -15.263590812683105 0 38.118125915534762
		 -15.263590812674806 0 46.724575042724609 -18.127838134765625 0 46.724575042731516
		 -18.127838134754462 0 55.638725280761719 -15 0 55.638725280765591 -14.999999999989555
		 0 0.43206751346588135 -30.28564453125 0 0.43206751347319217 -30.28564453124164 0 10.005419731140137
		 -30 0 10.005419731141668 -29.999999999992259 0 17.645288467407227 -30 0 17.645288467417782
		 -29.999999999991978 0 29.775415420532227 -30.263591766357422 0 29.775415420539648
		 -30.263591766349123 0 38.381866455078125 -33.127838134765625 0 38.381866455085031
		 -33.127838134754462 0 47.296016693115234 -30 0 47.296016693119107 -29.999999999989555
		 0 ;
createNode groupParts -n "vectorAdjust1GroupParts120";
	rename -uid "76735A93-7742-7E9A-479E-5F81BD9A8DD8";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode tweak -n "tweak121";
	rename -uid "BFB8CC81-F043-34B8-003B-D7A88029DF78";
createNode objectSet -n "tweakSet121";
	rename -uid "CADC017A-9546-51CD-8B43-0980043FF0E7";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId4198";
	rename -uid "D8BCD87E-F54E-CB9B-6B07-BFA3DDBF70D7";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts123";
	rename -uid "9FA7B9AC-984E-2F4B-DAE2-F18A1E014E15";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode typeExtrude -n "typeExtrude121";
	rename -uid "CA9AC3F2-7F4E-6D77-EA0C-CB8DF0503A9A";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
	setAttr ".exdv" 1;
	setAttr ".exds" 0;
	setAttr ".capComponents" -type "componentList" 24 "f[0]" "f[70:71]" "f[138:139]" "f[144:145]" "f[150:151]" "f[227:228]" "f[363:364]" "f[434:435]" "f[506:507]" "f[642:643]" "f[708:709]" "f[714:715]" "f[720:721]" "f[797:798]" "f[933:934]" "f[1004:1005]" "f[1020:1021]" "f[1086:1087]" "f[1092:1093]" "f[1098:1099]" "f[1175:1176]" "f[1311:1312]" "f[1382:1383]" "f[1398]";
	setAttr ".bevelComponents" -type "componentList" 0;
	setAttr ".extrusionComponents" -type "componentList" 23 "f[1:69]" "f[72:137]" "f[140:143]" "f[146:149]" "f[152:226]" "f[229:362]" "f[365:433]" "f[436:505]" "f[508:641]" "f[644:707]" "f[710:713]" "f[716:719]" "f[722:796]" "f[799:932]" "f[935:1003]" "f[1006:1019]" "f[1022:1085]" "f[1088:1091]" "f[1094:1097]" "f[1100:1174]" "f[1177:1310]" "f[1313:1381]" "f[1384:1397]";
	setAttr -s 31 ".charGroupId";
createNode type -n "type121";
	rename -uid "221BD3AF-D24F-1C66-0AE6-9297365EB527";
	setAttr ".solidsPerCharacter" -type "doubleArray" 20 1 1 2 1 1 1 1 1 1 2 1 1
		 1 1 1 2 1 1 1 1 ;
	setAttr ".solidsPerWord" -type "doubleArray" 9 4 4 4 4 3 4 0 0 0 ;
	setAttr ".solidsPerLine" -type "doubleArray" 6 8 8 7 0 0 0 ;
	setAttr ".vertsPerChar" -type "doubleArray" 20 69 135 143 218 352 421 491 625
		 689 697 772 906 975 989 1053 1061 1136 1270 1339 1353 ;
	setAttr ".characterBoundingBoxesMax" -type "vectorArray" 20 7.7721149285410824
		 10.797487125673934 0 15.49222721720435 8.0724616929280941 0 19.037731115213315 7.7428604032978789
		 0 35.70226083604372 8.0211538179793731 0 44.697196949718709 8.0358560511163866 0 52.790867858686418
		 8.0358560511163866 0 61.082269209849692 10.526544827560947 0 8.0211538179793731 -6.9641439488836152
		 0 16.085570201703881 -6.9275383070719077 0 19.879268855485233 -7.2571395967021228
		 0 36.543798576315638 -6.9788461820206287 0 45.538734689990619 -6.9641439488836152
		 0 53.632405598958329 -6.9641439488836152 0 59.513830514899794 -4.554166941367793
		 0 7.7428604032978789 -21.927538307071906 0 11.536559057079232 -22.257139596702125
		 0 28.201088777909636 -21.978846182020629 0 37.196024891584628 -21.964143948883617
		 0 45.289695800552337 -21.964143948883617 0 51.171120716493789 -19.554166941367793
		 0 ;
	setAttr ".characterBoundingBoxesMin" -type "vectorArray" 20 0.86428506800226357
		 -0.24903892520666684 0 8.7747773261233455 -0.22698548608020724 0 17.5065916824162
		 0 0 25.1464604101883 0 0 37.276587005629544 -0.26359119942866749 0 45.8830379981476
		 -3.1278388644404891 0 53.8302860179531 0 0 0.60054387389020747 -15.263591199428669
		 0 8.7747773261233455 -15.285644638555128 0 18.348129422688118 -15.000000000000002
		 0 25.987998150460218 -15.000000000000002 0 38.118124745901461 -15.263591199428669
		 0 46.724575738419517 -18.127838864440491 0 55.638724898040103 -15.000000000000002
		 0 0.43206752771734358 -30.28564463855513 0 10.005419624282116 -30.000000000000004
		 0 17.645288352054216 -30.000000000000004 0 29.775414947495459 -30.263591199428671
		 0 38.381865940013519 -33.127838864440491 0 47.296015099634104 -30.000000000000004
		 0 ;
	setAttr ".manipulatorPivots" -type "vectorArray" 20 0.86428506800226357 -0.24903892520666684
		 0 8.7747773261233455 -0.22698548608020724 0 17.5065916824162 0 0 25.1464604101883
		 0 0 37.276587005629544 -0.26359119942866749 0 45.8830379981476 -3.1278388644404891
		 0 53.8302860179531 0 0 0.60054387389020747 -15.263591199428669 0 8.7747773261233455
		 -15.285644638555128 0 18.348129422688118 -15.000000000000002 0 25.987998150460218
		 -15.000000000000002 0 38.118124745901461 -15.263591199428669 0 46.724575738419517
		 -18.127838864440491 0 55.638724898040103 -15.000000000000002 0 0.43206752771734358
		 -30.28564463855513 0 10.005419624282116 -30.000000000000004 0 17.645288352054216
		 -30.000000000000004 0 29.775414947495459 -30.263591199428671 0 38.381865940013519
		 -33.127838864440491 0 47.296015099634104 -30.000000000000004 0 ;
	setAttr ".holeInfo" -type "Int32Array" 30 0 32 37 5 34
		 318 6 32 389 8 34 591 9 32 657 13 34
		 872 14 32 943 16 32 1021 20 34 1236 21 32
		 1307 ;
	setAttr ".numberOfShells" 23;
	setAttr ".textInput" -type "string" "62 63 3A 20 6D 61 70 32 0A 61 6F 3A 20 6D 61 70 31 0A 6F 3A 20 6D 61 70 31 0A 0A 0A";
	setAttr ".currentFont" -type "string" "Helvetica";
	setAttr ".currentStyle" -type "string" "Regular";
	setAttr ".manipulatorPositionsPP" -type "vectorArray" 83 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordPositionsPP" -type "vectorArray" 9 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorLinePositionsPP" -type "vectorArray" 6 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorRotationsPP" -type "vectorArray" 83 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordRotationsPP" -type "vectorArray" 9 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorLineRotationsPP" -type "vectorArray" 6 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorScalesPP" -type "vectorArray" 83 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordScalesPP" -type "vectorArray" 9 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorLineScalesPP" -type "vectorArray" 6 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 ;
	setAttr ".alignmentAdjustments" -type "doubleArray" 6 0 0 0 0 0 0 ;
	setAttr ".manipulatorMode" 0;
	setAttr ".fontSize" 15;
createNode groupId -n "groupid361";
	rename -uid "B596D12B-4745-8927-373C-529D38B5AEDB";
createNode groupId -n "groupid362";
	rename -uid "90F6EB1C-2F44-58A7-8DAE-9A85099492A8";
createNode groupId -n "groupid363";
	rename -uid "6073E5C7-F247-7229-192B-59AFE6AFB49B";
createNode groupId -n "groupId4199";
	rename -uid "58EF148E-064B-E8CF-5F4E-83B8D9479366";
createNode groupId -n "groupId4200";
	rename -uid "5098B295-F04B-8C42-6E1B-F7BA51781C60";
createNode groupId -n "groupId4201";
	rename -uid "B93A0BC0-AD43-BC09-EEB4-B7AE41D6E976";
createNode groupId -n "groupId4202";
	rename -uid "F1616A58-E747-E969-3BE4-56A82120475F";
createNode groupId -n "groupId4203";
	rename -uid "A39BCFB7-8249-A7AB-EA46-8081FEA937BD";
createNode groupId -n "groupId4204";
	rename -uid "469D8BE8-6F4C-0895-7A0F-62ACBF204D0C";
createNode groupId -n "groupId4205";
	rename -uid "9B27BA69-0744-E768-6EEA-90BC15D79D6D";
createNode groupId -n "groupId4206";
	rename -uid "5508CF06-6D47-1E72-70A9-B09DBE441AAA";
createNode groupId -n "groupId4207";
	rename -uid "19F6F852-DD4B-6785-060B-DD89A293D6F5";
createNode groupId -n "groupId4208";
	rename -uid "51CA4226-BB4C-7556-10EF-A99417E0F4CD";
createNode groupId -n "groupId4209";
	rename -uid "F4DF38B0-EE4F-09E2-25D3-50A69707B328";
createNode groupId -n "groupId4210";
	rename -uid "C9A02502-264E-6BE3-898A-30819E1F4328";
createNode groupId -n "groupId4211";
	rename -uid "AF994E60-FF45-58FB-74D5-05BBFDA33F18";
createNode groupId -n "groupId4212";
	rename -uid "147A2A8E-F842-B813-0A03-B9B32EA878B9";
createNode groupId -n "groupId4213";
	rename -uid "7646F3F9-6341-0B02-694B-729B2F6B5827";
createNode groupId -n "groupId4214";
	rename -uid "2847B8A8-1648-C76A-DAFF-01B742E00AB4";
createNode groupId -n "groupId4215";
	rename -uid "7D70B3BB-2845-EAB3-04D2-B39486578A0B";
createNode groupId -n "groupId4216";
	rename -uid "6D5B184E-3C4F-4635-0F38-CB814F68D9B1";
createNode groupId -n "groupId4217";
	rename -uid "566F2EA4-3949-2532-B027-0F942DDBE1DC";
createNode groupId -n "groupId4218";
	rename -uid "CFC95430-BB4E-DCF8-56D9-63B67B84D518";
createNode groupId -n "groupId4219";
	rename -uid "42667F35-E946-06DA-3A91-72A58208186E";
createNode groupId -n "groupId4220";
	rename -uid "34588C52-EB49-4206-0D09-5282F9D6B5D3";
createNode groupId -n "groupId4221";
	rename -uid "86EAB412-7948-0D43-835E-69AF19E5F143";
createNode groupId -n "groupId4222";
	rename -uid "5E6A36AC-AE46-1DBA-8C11-99B0E38714E8";
createNode groupId -n "groupId4223";
	rename -uid "7257A9F6-8645-0955-2E4C-4B8B5A966295";
createNode groupId -n "groupId4224";
	rename -uid "EE607286-404F-0C72-1F6F-E88BC1DAA0A4";
createNode groupId -n "groupId4225";
	rename -uid "995ABAAA-6D43-F869-A0EB-C1B53611B118";
createNode groupId -n "groupId4226";
	rename -uid "8BDD9867-F442-D14C-F857-6DB7FAE2E5B7";
createNode groupId -n "groupId4227";
	rename -uid "2AA36EB2-9646-178B-D937-51AE071005B5";
createNode groupId -n "groupId4228";
	rename -uid "31005C18-254D-FE77-D68C-3F8E454954A2";
createNode groupId -n "groupId4229";
	rename -uid "B831D010-7140-C333-0CCA-98957BCA8632";
createNode groupId -n "shellDeformer1GroupId120";
	rename -uid "46D458DE-B64C-C349-539C-3CA17A280976";
	setAttr ".ihi" 0;
createNode objectSet -n "shellDeformer1Set120";
	rename -uid "55E3DFD2-BF40-EF4F-0F53-F6AE552C3428";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode shellDeformer -n "shellDeformer121";
	rename -uid "81E0934A-0940-1E55-24A2-EEA4D285C9D7";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
createNode groupParts -n "shellDeformer1GroupParts120";
	rename -uid "45DADA5B-1741-6660-3749-07BE9C64305C";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode polyAutoProj -n "polyAutoProj121";
	rename -uid "72729452-1C42-1B24-18BA-ECAA248AA465";
	setAttr ".uopa" yes;
	setAttr ".ics" -type "componentList" 1 "f[*]";
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
	setAttr ".ps" 0.20000000298023224;
	setAttr ".dl" yes;
createNode polyRemesh -n "polyRemesh121";
	rename -uid "92312592-F14C-A302-F1D3-D7A240668691";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
	setAttr ".nds" 1;
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
	setAttr ".mel" 0.71345293521881104;
	setAttr ".cot" 33.632286071777344;
	setAttr ".tsb" no;
	setAttr ".ipt" 0;
createNode polySoftEdge -n "polySoftEdge121";
	rename -uid "2BD8A0E6-2E46-9C03-45A2-9991EC66383D";
	setAttr ".uopa" yes;
	setAttr ".ics" -type "componentList" 1 "e[*]";
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
createNode polyTriangulate -n "polyTriangulate121";
	rename -uid "85735B95-5B4B-4EB8-143D-C6A76028CB80";
	setAttr ".ics" -type "componentList" 1 "f[*]";
createNode materialInfo -n "materialInfo107";
	rename -uid "0F7DF024-3145-F3BD-1C8E-B7BD2717B60F";
createNode shadingEngine -n "usdPreviewSurface2SG";
	rename -uid "F8AD78F7-C546-58FA-F89E-0C93AF51AF56";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;
createNode usdPreviewSurface -n "vistest_label_text_template_text_mat";
	rename -uid "9C39D5A3-F742-C188-64EC-258A98EF6DAD";
	setAttr ".dc" -type "float3" 0 0 0 ;
	setAttr ".ec" -type "float3" 0 1 1 ;
	setAttr ".ior" 1;
	setAttr ".mtl" 1;
	setAttr ".rgh" 1;
createNode lightLinker -s -n "lightLinker1";
	rename -uid "C233CC3A-FB42-9F49-0537-CFA48D35BE4E";
	setAttr -s 12 ".lnk";
	setAttr -s 12 ".slnk";
createNode shapeEditorManager -n "shapeEditorManager";
	rename -uid "9673361E-934E-0B02-ECD2-FBB4071895AC";
createNode poseInterpolatorManager -n "poseInterpolatorManager";
	rename -uid "2D6D9E47-2141-3C47-3A73-1A8B837C3335";
createNode displayLayerManager -n "layerManager";
	rename -uid "D795ECF5-C84A-3157-FA98-01B68A98BA98";
createNode displayLayer -n "defaultLayer";
	rename -uid "6A6D8E25-8842-07EB-1202-B7A5EA682ED6";
	setAttr ".ufem" -type "stringArray" 0  ;
createNode renderLayerManager -n "renderLayerManager";
	rename -uid "18194064-4042-3760-53DC-08BE98941FBB";
createNode renderLayer -n "defaultRenderLayer";
	rename -uid "32ABC7ED-0047-9531-FF9B-1582B5E3AF39";
	setAttr ".g" yes;
createNode transformGeometry -n "transformGeometry1";
	rename -uid "440F1A30-4C46-1515-2275-8896CC392341";
	setAttr ".txf" -type "matrix" 0.050000000000000003 0 0 0 0 0.050000000000000003 0 0
		 0 0 0.050000000000000003 0 -4.9999923706054688 -6.4689668792141788 0 1;
createNode script -n "uiConfigurationScriptNode";
	rename -uid "371F1698-884A-D6B1-6439-BBA89FB6C021";
	setAttr ".b" -type "string" (
		"// Maya Mel UI Configuration File.\n//\n//  This script is machine generated.  Edit at your own risk.\n//\n//\n\nglobal string $gMainPane;\nif (`paneLayout -exists $gMainPane`) {\n\n\tglobal int $gUseScenePanelConfig;\n\tint    $useSceneConfig = $gUseScenePanelConfig;\n\tint    $nodeEditorPanelVisible = stringArrayContains(\"nodeEditorPanel1\", `getPanel -vis`);\n\tint    $nodeEditorWorkspaceControlOpen = (`workspaceControl -exists nodeEditorPanel1Window` && `workspaceControl -q -visible nodeEditorPanel1Window`);\n\tint    $menusOkayInPanels = `optionVar -q allowMenusInPanels`;\n\tint    $nVisPanes = `paneLayout -q -nvp $gMainPane`;\n\tint    $nPanes = 0;\n\tstring $editorName;\n\tstring $panelName;\n\tstring $itemFilterName;\n\tstring $panelConfig;\n\n\t//\n\t//  get current state of the UI\n\t//\n\tsceneUIReplacement -update $gMainPane;\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Top View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Top View\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        modelEditor -e \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n"
		+ "            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n"
		+ "            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -bluePencil 1\n            -greasePencils 0\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n"
		+ "\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Side View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Side View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n"
		+ "            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n"
		+ "            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -bluePencil 1\n            -greasePencils 0\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n"
		+ "            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Front View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Front View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n"
		+ "            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n"
		+ "            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n"
		+ "            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -bluePencil 1\n            -greasePencils 0\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Persp View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Persp View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"|persp\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"wireframe\" \n            -activeOnly 0\n"
		+ "            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 1\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n"
		+ "            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n"
		+ "            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -bluePencil 1\n            -greasePencils 0\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1614\n            -height 974\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"ToggledOutliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n"
		+ "\t\toutlinerPanel -edit -l (localizedPanelLabel(\"ToggledOutliner\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n            -showReferenceNodes 1\n            -showReferenceMembers 1\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 1\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n            -showParentContainers 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n"
		+ "            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n            -isSet 0\n            -isSetMember 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n            -longNames 0\n            -niceNames 1\n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            -renderFilterIndex 0\n"
		+ "            -selectionOrder \"chronological\" \n            -expandAttribute 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"Outliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"Outliner\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n            -showReferenceNodes 0\n            -showReferenceMembers 0\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 1\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n"
		+ "            -showParentContainers 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n"
		+ "            -longNames 0\n            -niceNames 1\n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"graphEditor\" (localizedPanelLabel(\"Graph Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Graph Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showAssignedMaterials 0\n                -showTimeEditor 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n"
		+ "                -organizeByClip 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 1\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showParentContainers 0\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 1\n                -showCompounds 0\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 1\n                -doNotSelectNewObjects 0\n                -dropIsParent 1\n                -transmitFilters 1\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n"
		+ "                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 1\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                -renderFilterVisible 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"GraphEd\");\n            animCurveEditor -e \n                -displayValues 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n                -showPlayRangeShades \"on\" \n                -lockPlayRangeShades \"off\" \n                -smoothness \"fine\" \n                -resultSamples 1\n"
		+ "                -resultScreenSamples 0\n                -resultUpdate \"delayed\" \n                -showUpstreamCurves 1\n                -keyMinScale 1\n                -stackedCurvesMin -1\n                -stackedCurvesMax 1\n                -stackedCurvesSpace 0.2\n                -preSelectionHighlight 0\n                -constrainDrag 0\n                -valueLinesToggle 1\n                -highlightAffectedCurves 0\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dopeSheetPanel\" (localizedPanelLabel(\"Dope Sheet\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dope Sheet\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showAssignedMaterials 0\n                -showTimeEditor 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n"
		+ "                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -organizeByClip 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 0\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showParentContainers 0\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 0\n                -showCompounds 1\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 0\n                -doNotSelectNewObjects 1\n                -dropIsParent 1\n                -transmitFilters 0\n                -setFilter \"0\" \n                -showSetMembers 0\n"
		+ "                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 0\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                -renderFilterVisible 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"DopeSheetEd\");\n            dopeSheetEditor -e \n                -displayValues 0\n                -snapTime \"integer\" \n"
		+ "                -snapValue \"none\" \n                -outliner \"dopeSheetPanel1OutlineEd\" \n                -showSummary 1\n                -showScene 0\n                -hierarchyBelow 0\n                -showTicks 1\n                -selectionWindow 0 0 0 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"timeEditorPanel\" (localizedPanelLabel(\"Time Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Time Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"clipEditorPanel\" (localizedPanelLabel(\"Trax Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Trax Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = clipEditorNameFromPanel($panelName);\n"
		+ "            clipEditor -e \n                -displayValues 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -initialized 0\n                -manageSequencer 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"sequenceEditorPanel\" (localizedPanelLabel(\"Camera Sequencer\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Camera Sequencer\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = sequenceEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayValues 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -initialized 0\n                -manageSequencer 1 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperGraphPanel\" (localizedPanelLabel(\"Hypergraph Hierarchy\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypergraph Hierarchy\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"HyperGraphEd\");\n            hyperGraph -e \n                -graphLayoutStyle \"hierarchicalLayout\" \n                -orientation \"horiz\" \n                -mergeConnections 0\n                -zoom 1\n                -animateTransition 0\n                -showRelationships 1\n                -showShapes 0\n                -showDeformers 0\n                -showExpressions 0\n                -showConstraints 0\n                -showConnectionFromSelected 0\n                -showConnectionToSelected 0\n                -showConstraintLabels 0\n                -showUnderworld 0\n                -showInvisible 0\n                -transitionFrames 1\n                -opaqueContainers 0\n                -freeform 0\n                -imagePosition 0 0 \n                -imageScale 1\n                -imageEnabled 0\n                -graphType \"DAG\" \n"
		+ "                -heatMapDisplay 0\n                -updateSelection 1\n                -updateNodeAdded 1\n                -useDrawOverrideColor 0\n                -limitGraphTraversal -1\n                -range 0 0 \n                -iconSize \"smallIcons\" \n                -showCachedConnections 0\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperShadePanel\" (localizedPanelLabel(\"Hypershade\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypershade\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"visorPanel\" (localizedPanelLabel(\"Visor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Visor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n"
		+ "\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"nodeEditorPanel\" (localizedPanelLabel(\"Node Editor\")) `;\n\tif ($nodeEditorPanelVisible || $nodeEditorWorkspaceControlOpen) {\n\t\tif (\"\" == $panelName) {\n\t\t\tif ($useSceneConfig) {\n\t\t\t\t$panelName = `scriptedPanel -unParent  -type \"nodeEditorPanel\" -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels `;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -connectNodeOnCreation 0\n                -connectOnDrop 0\n                -copyConnectionsOnPaste 0\n                -connectionStyle \"bezier\" \n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -connectedGraphingMode 1\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n"
		+ "                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -crosshairOnEdgeDragging 0\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -showUnitConversions 0\n                -editorMode \"default\" \n                -hasWatchpoint 0\n                $editorName;\n\t\t\t}\n\t\t} else {\n\t\t\t$label = `panel -q -label $panelName`;\n\t\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n"
		+ "                -connectNodeOnCreation 0\n                -connectOnDrop 0\n                -copyConnectionsOnPaste 0\n                -connectionStyle \"bezier\" \n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -connectedGraphingMode 1\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -crosshairOnEdgeDragging 0\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -showUnitConversions 0\n                -editorMode \"default\" \n                -hasWatchpoint 0\n                $editorName;\n\t\t\tif (!$useSceneConfig) {\n"
		+ "\t\t\t\tpanel -e -l $label $panelName;\n\t\t\t}\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"createNodePanel\" (localizedPanelLabel(\"Create Node\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Create Node\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"polyTexturePlacementPanel\" (localizedPanelLabel(\"UV Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"UV Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"renderWindowPanel\" (localizedPanelLabel(\"Render View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Render View\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"shapePanel\" (localizedPanelLabel(\"Shape Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tshapePanel -edit -l (localizedPanelLabel(\"Shape Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"posePanel\" (localizedPanelLabel(\"Pose Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tposePanel -edit -l (localizedPanelLabel(\"Pose Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynRelEdPanel\" (localizedPanelLabel(\"Dynamic Relationships\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dynamic Relationships\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"relationshipPanel\" (localizedPanelLabel(\"Relationship Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Relationship Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"referenceEditorPanel\" (localizedPanelLabel(\"Reference Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Reference Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynPaintScriptedPanelType\" (localizedPanelLabel(\"Paint Effects\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Paint Effects\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"scriptEditorPanel\" (localizedPanelLabel(\"Script Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Script Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"profilerPanel\" (localizedPanelLabel(\"Profiler Tool\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Profiler Tool\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"contentBrowserPanel\" (localizedPanelLabel(\"Content Browser\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Content Browser\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"Stereo\" (localizedPanelLabel(\"Stereo\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Stereo\")) -mbv $menusOkayInPanels  $panelName;\n{ string $editorName = ($panelName+\"Editor\");\n            stereoCameraView -e \n                -camera \"|persp\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"wireframe\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 1\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n"
		+ "                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 4 4 \n                -bumpResolution 4 4 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 0\n                -occlusionCulling 0\n                -shadingModel 0\n"
		+ "                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -controllers 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n"
		+ "                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -bluePencil 1\n                -greasePencils 0\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 0\n                -height 0\n                -sceneRenderFilter 0\n                -displayMode \"centerEye\" \n                -viewColor 0 0 0 1 \n                -useCustomBackground 1\n                $editorName;\n            stereoCameraView -e -viewSelected 0 $editorName;\n            stereoCameraView -e \n                -pluginObjects \"gpuCacheDisplayFilter\" 1 \n                $editorName; };\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\tif ($useSceneConfig) {\n        string $configName = `getPanel -cwl (localizedPanelLabel(\"Current Layout\"))`;\n        if (\"\" != $configName) {\n\t\t\tpanelConfiguration -edit -label (localizedPanelLabel(\"Current Layout\")) \n"
		+ "\t\t\t\t-userCreated false\n\t\t\t\t-defaultImage \"vacantCell.xP:/\"\n\t\t\t\t-image \"\"\n\t\t\t\t-sc false\n\t\t\t\t-configString \"global string $gMainPane; paneLayout -e -cn \\\"single\\\" -ps 1 100 100 $gMainPane;\"\n\t\t\t\t-removeAllPanels\n\t\t\t\t-ap false\n\t\t\t\t\t(localizedPanelLabel(\"Persp View\")) \n\t\t\t\t\t\"modelPanel\"\n"
		+ "\t\t\t\t\t\"$panelName = `modelPanel -unParent -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels `;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"wireframe\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 1\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 16384\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"vp2Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -controllers 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -bluePencil 1\\n    -greasePencils 0\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 1614\\n    -height 974\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName;\\nmodelEditor -e \\n    -pluginObjects \\\"gpuCacheDisplayFilter\\\" 1 \\n    $editorName\"\n"
		+ "\t\t\t\t\t\"modelPanel -edit -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels  $panelName;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"wireframe\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 1\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 16384\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"vp2Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -controllers 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -bluePencil 1\\n    -greasePencils 0\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 1614\\n    -height 974\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName;\\nmodelEditor -e \\n    -pluginObjects \\\"gpuCacheDisplayFilter\\\" 1 \\n    $editorName\"\n"
		+ "\t\t\t\t$configName;\n\n            setNamedPanelLayout (localizedPanelLabel(\"Current Layout\"));\n        }\n\n        panelHistory -e -clear mainPanelHistory;\n        sceneUIReplacement -clear;\n\t}\n\n\ngrid -spacing 5 -size 12 -divisions 5 -displayAxes yes -displayGridLines yes -displayDivisionLines yes -displayPerspectiveLabels no -displayOrthographicLabels no -displayAxesBold yes -perspectiveLabelPosition axis -orthographicLabelPosition edge;\nviewManip -drawCompass 0 -compassAngle 0 -frontParameters \"\" -homeParameters \"\" -selectionLockParameters \"\";\n}\n");
	setAttr ".st" 3;
createNode script -n "sceneConfigurationScriptNode";
	rename -uid "03C6C8E0-824D-32A0-ED89-9B93658B935A";
	setAttr ".b" -type "string" "playbackOptions -min 0.8 -max 120 -ast 0.8 -aet 200 ";
	setAttr ".st" 6;
select -ne :time1;
	setAttr -av -k on ".cch";
	setAttr -av -cb on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k on ".o" 69;
	setAttr -av -k on ".unw" 69;
	setAttr -av -k on ".etw";
	setAttr -av -k on ".tps";
	setAttr -av -k on ".tms";
select -ne :hardwareRenderingGlobals;
	setAttr -av -k on ".ihi";
	setAttr ".otfna" -type "stringArray" 22 "NURBS Curves" "NURBS Surfaces" "Polygons" "Subdiv Surface" "Particles" "Particle Instance" "Fluids" "Strokes" "Image Planes" "UI" "Lights" "Cameras" "Locators" "Joints" "IK Handles" "Deformers" "Motion Trails" "Components" "Hair Systems" "Follicles" "Misc. UI" "Ornaments"  ;
	setAttr ".otfva" -type "Int32Array" 22 0 1 1 1 1 1
		 1 1 1 0 0 0 0 0 0 0 0 0
		 0 0 0 0 ;
	setAttr -k on ".hwi";
	setAttr -av ".ta";
	setAttr -av ".tq";
	setAttr ".etmr" no;
	setAttr ".tmr" 4096;
	setAttr -av ".aoam";
	setAttr -av ".aora";
	setAttr -av ".hfd";
	setAttr -av ".hfe";
	setAttr -av ".hfcr";
	setAttr -av ".hfcg";
	setAttr -av ".hfcb";
	setAttr -av ".hfa";
	setAttr -av ".mbe";
	setAttr -av -k on ".mbsof";
	setAttr -k on ".blen";
	setAttr -k on ".blat";
	setAttr ".fprt" yes;
select -ne :renderPartition;
	setAttr -av -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 4 ".st";
	setAttr -cb on ".an";
	setAttr -cb on ".pt";
select -ne :renderGlobalsList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
select -ne :defaultShaderList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 7 ".s";
select -ne :postProcessList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 2 ".p";
select -ne :defaultRenderUtilityList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 3 ".u";
select -ne :defaultRenderingList1;
	setAttr -k on ".ihi";
select -ne :defaultTextureList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 3 ".tx";
select -ne :standardSurface1;
	setAttr ".b" 0.80000001192092896;
	setAttr ".bc" -type "float3" 1 1 1 ;
	setAttr ".s" 0.20000000298023224;
select -ne :initialShadingGroup;
	setAttr -av -k on ".cch";
	setAttr -k on ".fzn";
	setAttr -cb on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k on ".bbx";
	setAttr -k on ".vwm";
	setAttr -k on ".tpv";
	setAttr -k on ".uit";
	setAttr -k on ".mwc";
	setAttr -cb on ".an";
	setAttr -cb on ".il";
	setAttr -cb on ".vo";
	setAttr -cb on ".eo";
	setAttr -cb on ".fo";
	setAttr -cb on ".epo";
	setAttr -k on ".ro" yes;
select -ne :initialParticleSE;
	setAttr -av -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k on ".mwc";
	setAttr -cb on ".an";
	setAttr -cb on ".il";
	setAttr -cb on ".vo";
	setAttr -cb on ".eo";
	setAttr -cb on ".fo";
	setAttr -cb on ".epo";
	setAttr -k on ".ro" yes;
select -ne :defaultRenderGlobals;
	addAttr -ci true -h true -sn "dss" -ln "defaultSurfaceShader" -dt "string";
	setAttr -av -k on ".cch";
	setAttr -k on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -k on ".bnm";
	setAttr -k on ".macc";
	setAttr -k on ".macd";
	setAttr -k on ".macq";
	setAttr -k on ".mcfr";
	setAttr -k on ".ifg";
	setAttr -k on ".clip";
	setAttr -k on ".edm";
	setAttr -av -k on ".edl";
	setAttr -av -k on ".esr";
	setAttr -k on ".ors";
	setAttr -k on ".sdf";
	setAttr -av -k on ".outf";
	setAttr -av -k on ".imfkey" -type "string" "iff";
	setAttr -av -k on ".gama";
	setAttr -k on ".exrc";
	setAttr -k on ".expt";
	setAttr -av -k on ".an";
	setAttr -k on ".ar";
	setAttr -k on ".fs";
	setAttr -k on ".ef";
	setAttr -av -k on ".bfs";
	setAttr -k on ".me";
	setAttr -k on ".se";
	setAttr -av -k on ".be";
	setAttr -av -k on ".ep";
	setAttr -k on ".fec";
	setAttr -av -k on ".ofc";
	setAttr -k on ".ofe";
	setAttr -k on ".efe";
	setAttr -k on ".oft";
	setAttr -k on ".umfn";
	setAttr -k on ".ufe";
	setAttr -av -k on ".pff";
	setAttr -av -k on ".peie";
	setAttr -av -k on ".ifp";
	setAttr -k on ".rv";
	setAttr -k on ".comp";
	setAttr -k on ".cth";
	setAttr -k on ".soll";
	setAttr -cb on ".sosl";
	setAttr -k on ".rd";
	setAttr -k on ".lp";
	setAttr -av -k on ".sp";
	setAttr -k on ".shs";
	setAttr -av -k on ".lpr";
	setAttr -k on ".gv";
	setAttr -k on ".sv";
	setAttr -k on ".mm";
	setAttr -k on ".npu";
	setAttr -k on ".itf";
	setAttr -k on ".shp";
	setAttr -k on ".isp";
	setAttr -k on ".uf";
	setAttr -k on ".oi";
	setAttr -k on ".rut";
	setAttr -k on ".mot";
	setAttr -av -k on ".mb";
	setAttr -av -k on ".mbf";
	setAttr -k on ".mbso";
	setAttr -k on ".mbsc";
	setAttr -av -k on ".afp";
	setAttr -k on ".pfb";
	setAttr -k on ".pram";
	setAttr -k on ".poam";
	setAttr -k on ".prlm";
	setAttr -k on ".polm";
	setAttr -k on ".prm";
	setAttr -k on ".pom";
	setAttr -k on ".pfrm";
	setAttr -k on ".pfom";
	setAttr -av -k on ".bll";
	setAttr -av -k on ".bls";
	setAttr -av -k on ".smv";
	setAttr -k on ".ubc";
	setAttr -k on ".mbc";
	setAttr -k on ".mbt";
	setAttr -k on ".udbx";
	setAttr -k on ".smc";
	setAttr -k on ".kmv";
	setAttr -k on ".isl";
	setAttr -k on ".ism";
	setAttr -k on ".imb";
	setAttr -av -k on ".rlen";
	setAttr -av -k on ".frts";
	setAttr -k on ".tlwd";
	setAttr -k on ".tlht";
	setAttr -k on ".jfc";
	setAttr -k on ".rsb";
	setAttr -k on ".ope";
	setAttr -k on ".oppf";
	setAttr -av -k on ".rcp";
	setAttr -av -k on ".icp";
	setAttr -av -k on ".ocp";
	setAttr -k on ".hbl";
	setAttr ".dss" -type "string" "lambert1";
select -ne :defaultResolution;
	setAttr -av -k on ".cch";
	setAttr -av -k on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -k on ".bnm";
	setAttr -av -k on ".w";
	setAttr -av -k on ".h";
	setAttr -av -k on ".pa" 1;
	setAttr -av -k on ".al";
	setAttr -av -k on ".dar";
	setAttr -av -k on ".ldar";
	setAttr -av -cb on ".dpi";
	setAttr -av -k on ".off";
	setAttr -av -k on ".fld";
	setAttr -av -k on ".zsl";
	setAttr -av -cb on ".isu";
	setAttr -av -cb on ".pdu";
select -ne :defaultColorMgtGlobals;
	setAttr ".cme" no;
	setAttr ".cfe" yes;
	setAttr ".cfp" -type "string" "<MAYA_RESOURCES>/OCIO-configs/Maya2022-default/config.ocio";
	setAttr ".vtn" -type "string" "ACES 1.0 SDR-video (sRGB)";
	setAttr ".vn" -type "string" "ACES 1.0 SDR-video";
	setAttr ".dn" -type "string" "sRGB";
	setAttr ".wsn" -type "string" "ACEScg";
	setAttr ".otn" -type "string" "ACES 1.0 SDR-video (sRGB)";
	setAttr ".potn" -type "string" "ACES 1.0 SDR-video (sRGB)";
select -ne :hardwareRenderGlobals;
	setAttr -av -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -av -k off -cb on ".ctrs" 256;
	setAttr -av -k off -cb on ".btrs" 512;
	setAttr -av -k off -cb on ".fbfm";
	setAttr -av -k off -cb on ".ehql";
	setAttr -av -k off -cb on ".eams";
	setAttr -av -k off -cb on ".eeaa";
	setAttr -av -k off -cb on ".engm";
	setAttr -av -k off -cb on ".mes";
	setAttr -av -k off -cb on ".emb";
	setAttr -av -k off -cb on ".mbbf";
	setAttr -av -k off -cb on ".mbs";
	setAttr -av -k off -cb on ".trm";
	setAttr -av -k off -cb on ".tshc";
	setAttr -av -k off -cb on ".enpt";
	setAttr -av -k off -cb on ".clmt";
	setAttr -av -k off -cb on ".tcov";
	setAttr -av -k off -cb on ".lith";
	setAttr -av -k off -cb on ".sobc";
	setAttr -av -k off -cb on ".cuth";
	setAttr -av -k off -cb on ".hgcd";
	setAttr -av -k off -cb on ".hgci";
	setAttr -av -k off -cb on ".mgcs";
	setAttr -av -k off -cb on ".twa";
	setAttr -av -k off -cb on ".twz";
	setAttr -k on ".hwcc";
	setAttr -k on ".hwdp";
	setAttr -k on ".hwql";
	setAttr -k on ".hwfr";
	setAttr -k on ".soll";
	setAttr -k on ".sosl";
	setAttr -k on ".bswa";
	setAttr -k on ".shml";
	setAttr -k on ".hwel";
select -ne :ikSystem;
	setAttr -s 4 ".sol";
connectAttr "vectorAdjust1GroupId120.id" "label_utility_linear_srgb4Shape.iog.og[0].gid"
		;
connectAttr "vectorAdjust1Set120.mwc" "label_utility_linear_srgb4Shape.iog.og[0].gco"
		;
connectAttr "groupId4198.id" "label_utility_linear_srgb4Shape.iog.og[1].gid";
connectAttr "tweakSet121.mwc" "label_utility_linear_srgb4Shape.iog.og[1].gco";
connectAttr "shellDeformer1GroupId120.id" "label_utility_linear_srgb4Shape.iog.og[2].gid"
		;
connectAttr "shellDeformer1Set120.mwc" "label_utility_linear_srgb4Shape.iog.og[2].gco"
		;
connectAttr "transformGeometry1.og" "label_utility_linear_srgb4Shape.i";
connectAttr "multi_uv_set_matSG.msg" "materialInfo40.sg";
connectAttr "multi_uv_set_mat.msg" "materialInfo40.m";
connectAttr "texcoord_checker_up_bc.msg" "materialInfo40.t" -na;
connectAttr "multi_uv_set_mat.oc" "multi_uv_set_matSG.ss";
connectAttr "mesh_template24xShape.iog" "multi_uv_set_matSG.dsm" -na;
connectAttr "texcoord_checker_up_bc.oc" "multi_uv_set_mat.dc";
connectAttr "uv_grid_ao.ocr" "multi_uv_set_mat.ocl";
connectAttr "uv_grid_ao_opacity.ocr" "multi_uv_set_mat.opc";
connectAttr ":defaultColorMgtGlobals.cme" "texcoord_checker_up_bc.cme";
connectAttr ":defaultColorMgtGlobals.cfe" "texcoord_checker_up_bc.cmcf";
connectAttr ":defaultColorMgtGlobals.cfp" "texcoord_checker_up_bc.cmcp";
connectAttr ":defaultColorMgtGlobals.wsn" "texcoord_checker_up_bc.ws";
connectAttr "place2dTexture25.c" "texcoord_checker_up_bc.c";
connectAttr "place2dTexture25.tf" "texcoord_checker_up_bc.tf";
connectAttr "place2dTexture25.rf" "texcoord_checker_up_bc.rf";
connectAttr "place2dTexture25.mu" "texcoord_checker_up_bc.mu";
connectAttr "place2dTexture25.mv" "texcoord_checker_up_bc.mv";
connectAttr "place2dTexture25.s" "texcoord_checker_up_bc.s";
connectAttr "place2dTexture25.wu" "texcoord_checker_up_bc.wu";
connectAttr "place2dTexture25.wv" "texcoord_checker_up_bc.wv";
connectAttr "place2dTexture25.re" "texcoord_checker_up_bc.re";
connectAttr "place2dTexture25.of" "texcoord_checker_up_bc.of";
connectAttr "place2dTexture25.r" "texcoord_checker_up_bc.ro";
connectAttr "place2dTexture25.n" "texcoord_checker_up_bc.n";
connectAttr "place2dTexture25.vt1" "texcoord_checker_up_bc.vt1";
connectAttr "place2dTexture25.vt2" "texcoord_checker_up_bc.vt2";
connectAttr "place2dTexture25.vt3" "texcoord_checker_up_bc.vt3";
connectAttr "place2dTexture25.vc1" "texcoord_checker_up_bc.vc1";
connectAttr "place2dTexture25.o" "texcoord_checker_up_bc.uv";
connectAttr "place2dTexture25.ofs" "texcoord_checker_up_bc.fs";
connectAttr "uvChooser1.ov1" "place2dTexture25.vt1";
connectAttr "uvChooser1.ov2" "place2dTexture25.vt2";
connectAttr "uvChooser1.ov3" "place2dTexture25.vt3";
connectAttr "uvChooser1.oc1" "place2dTexture25.vc1";
connectAttr "uvChooser1.ouv" "place2dTexture25.uv";
connectAttr "mesh_template24xShape.uvst[1].uvsn" "uvChooser1.uvs[1]";
connectAttr ":defaultColorMgtGlobals.cme" "uv_grid_ao.cme";
connectAttr ":defaultColorMgtGlobals.cfe" "uv_grid_ao.cmcf";
connectAttr ":defaultColorMgtGlobals.cfp" "uv_grid_ao.cmcp";
connectAttr ":defaultColorMgtGlobals.wsn" "uv_grid_ao.ws";
connectAttr "place2dTexture26.c" "uv_grid_ao.c";
connectAttr "place2dTexture26.tf" "uv_grid_ao.tf";
connectAttr "place2dTexture26.rf" "uv_grid_ao.rf";
connectAttr "place2dTexture26.mu" "uv_grid_ao.mu";
connectAttr "place2dTexture26.mv" "uv_grid_ao.mv";
connectAttr "place2dTexture26.s" "uv_grid_ao.s";
connectAttr "place2dTexture26.wu" "uv_grid_ao.wu";
connectAttr "place2dTexture26.wv" "uv_grid_ao.wv";
connectAttr "place2dTexture26.re" "uv_grid_ao.re";
connectAttr "place2dTexture26.of" "uv_grid_ao.of";
connectAttr "place2dTexture26.r" "uv_grid_ao.ro";
connectAttr "place2dTexture26.n" "uv_grid_ao.n";
connectAttr "place2dTexture26.vt1" "uv_grid_ao.vt1";
connectAttr "place2dTexture26.vt2" "uv_grid_ao.vt2";
connectAttr "place2dTexture26.vt3" "uv_grid_ao.vt3";
connectAttr "place2dTexture26.vc1" "uv_grid_ao.vc1";
connectAttr "place2dTexture26.o" "uv_grid_ao.uv";
connectAttr "place2dTexture26.ofs" "uv_grid_ao.fs";
connectAttr ":defaultColorMgtGlobals.cme" "uv_grid_ao_opacity.cme";
connectAttr ":defaultColorMgtGlobals.cfe" "uv_grid_ao_opacity.cmcf";
connectAttr ":defaultColorMgtGlobals.cfp" "uv_grid_ao_opacity.cmcp";
connectAttr ":defaultColorMgtGlobals.wsn" "uv_grid_ao_opacity.ws";
connectAttr "place2dTexture41.c" "uv_grid_ao_opacity.c";
connectAttr "place2dTexture41.tf" "uv_grid_ao_opacity.tf";
connectAttr "place2dTexture41.rf" "uv_grid_ao_opacity.rf";
connectAttr "place2dTexture41.mu" "uv_grid_ao_opacity.mu";
connectAttr "place2dTexture41.mv" "uv_grid_ao_opacity.mv";
connectAttr "place2dTexture41.s" "uv_grid_ao_opacity.s";
connectAttr "place2dTexture41.wu" "uv_grid_ao_opacity.wu";
connectAttr "place2dTexture41.wv" "uv_grid_ao_opacity.wv";
connectAttr "place2dTexture41.re" "uv_grid_ao_opacity.re";
connectAttr "place2dTexture41.of" "uv_grid_ao_opacity.of";
connectAttr "place2dTexture41.r" "uv_grid_ao_opacity.ro";
connectAttr "place2dTexture41.n" "uv_grid_ao_opacity.n";
connectAttr "place2dTexture41.vt1" "uv_grid_ao_opacity.vt1";
connectAttr "place2dTexture41.vt2" "uv_grid_ao_opacity.vt2";
connectAttr "place2dTexture41.vt3" "uv_grid_ao_opacity.vt3";
connectAttr "place2dTexture41.vc1" "uv_grid_ao_opacity.vc1";
connectAttr "place2dTexture41.o" "uv_grid_ao_opacity.uv";
connectAttr "place2dTexture41.ofs" "uv_grid_ao_opacity.fs";
connectAttr "vectorAdjust1GroupId120.msg" "vectorAdjust1Set120.gn" -na;
connectAttr "label_utility_linear_srgb4Shape.iog.og[0]" "vectorAdjust1Set120.dsm"
		 -na;
connectAttr "vectorAdjust121.msg" "vectorAdjust1Set120.ub[0]";
connectAttr "vectorAdjust1GroupParts120.og" "vectorAdjust121.ip[0].ig";
connectAttr "vectorAdjust1GroupId120.id" "vectorAdjust121.ip[0].gi";
connectAttr "type121.grouping" "vectorAdjust121.grouping";
connectAttr "type121.manipulatorTransforms" "vectorAdjust121.manipulatorTransforms"
		;
connectAttr "type121.alignmentMode" "vectorAdjust121.alignmentMode";
connectAttr "type121.vertsPerChar" "vectorAdjust121.vertsPerChar";
connectAttr "typeExtrude121.vertexGroupIds" "vectorAdjust121.vertexGroupIds";
connectAttr "tweak121.og[0]" "vectorAdjust1GroupParts120.ig";
connectAttr "vectorAdjust1GroupId120.id" "vectorAdjust1GroupParts120.gi";
connectAttr "groupParts123.og" "tweak121.ip[0].ig";
connectAttr "groupId4198.id" "tweak121.ip[0].gi";
connectAttr "groupId4198.msg" "tweakSet121.gn" -na;
connectAttr "label_utility_linear_srgb4Shape.iog.og[1]" "tweakSet121.dsm" -na;
connectAttr "tweak121.msg" "tweakSet121.ub[0]";
connectAttr "typeExtrude121.out" "groupParts123.ig";
connectAttr "groupId4198.id" "groupParts123.gi";
connectAttr "type121.vertsPerChar" "typeExtrude121.vertsPerChar";
connectAttr "groupid361.id" "typeExtrude121.cid";
connectAttr "groupid362.id" "typeExtrude121.bid";
connectAttr "groupid363.id" "typeExtrude121.eid";
connectAttr "type121.outputMesh" "typeExtrude121.in";
connectAttr "type121.extrudeMessage" "typeExtrude121.typeMessage";
connectAttr "groupId4199.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4200.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4201.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4202.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4203.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4204.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4205.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4206.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4207.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4208.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4209.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4210.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4211.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4212.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4213.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4214.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4215.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4216.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4217.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4218.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4219.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4220.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4221.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4222.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4223.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4224.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4225.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4226.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4227.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4228.id" "typeExtrude121.charGroupId" -na;
connectAttr "groupId4229.id" "typeExtrude121.charGroupId" -na;
connectAttr "label_utility_linear_srgb4.msg" "type121.transformMessage";
connectAttr "shellDeformer1GroupId120.msg" "shellDeformer1Set120.gn" -na;
connectAttr "label_utility_linear_srgb4Shape.iog.og[2]" "shellDeformer1Set120.dsm"
		 -na;
connectAttr "shellDeformer121.msg" "shellDeformer1Set120.ub[0]";
connectAttr "shellDeformer1GroupParts120.og" "shellDeformer121.ip[0].ig";
connectAttr "shellDeformer1GroupId120.id" "shellDeformer121.ip[0].gi";
connectAttr "type121.vertsPerChar" "shellDeformer121.vertsPerChar";
connectAttr ":time1.o" "shellDeformer121.ti";
connectAttr "type121.grouping" "shellDeformer121.grouping";
connectAttr "type121.animationMessage" "shellDeformer121.typeMessage";
connectAttr "typeExtrude121.vertexGroupIds" "shellDeformer121.vertexGroupIds";
connectAttr "polyAutoProj121.out" "shellDeformer1GroupParts120.ig";
connectAttr "shellDeformer1GroupId120.id" "shellDeformer1GroupParts120.gi";
connectAttr "polyRemesh121.out" "polyAutoProj121.ip";
connectAttr "label_utility_linear_srgb4Shape.wm" "polyAutoProj121.mp";
connectAttr "polySoftEdge121.out" "polyRemesh121.ip";
connectAttr "label_utility_linear_srgb4Shape.wm" "polyRemesh121.mp";
connectAttr "type121.remeshMessage" "polyRemesh121.typeMessage";
connectAttr "typeExtrude121.capComponents" "polyRemesh121.ics";
connectAttr "vectorAdjust121.og[0]" "polySoftEdge121.ip";
connectAttr "label_utility_linear_srgb4Shape.wm" "polySoftEdge121.mp";
connectAttr "shellDeformer121.og[0]" "polyTriangulate121.ip";
connectAttr "usdPreviewSurface2SG.msg" "materialInfo107.sg";
connectAttr "vistest_label_text_template_text_mat.msg" "materialInfo107.m";
connectAttr "vistest_label_text_template_text_mat.msg" "materialInfo107.t" -na;
connectAttr "vistest_label_text_template_text_mat.oc" "usdPreviewSurface2SG.ss";
connectAttr "label_utility_linear_srgb4Shape.iog" "usdPreviewSurface2SG.dsm" -na
		;
relationship "link" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "multi_uv_set_matSG.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "usdPreviewSurface2SG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "multi_uv_set_matSG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "usdPreviewSurface2SG.message" ":defaultLightSet.message";
connectAttr "layerManager.dli[0]" "defaultLayer.id";
connectAttr "renderLayerManager.rlmi[0]" "defaultRenderLayer.rlid";
connectAttr "polyTriangulate121.out" "transformGeometry1.ig";
connectAttr "multi_uv_set_matSG.pa" ":renderPartition.st" -na;
connectAttr "usdPreviewSurface2SG.pa" ":renderPartition.st" -na;
connectAttr "multi_uv_set_mat.msg" ":defaultShaderList1.s" -na;
connectAttr "vistest_label_text_template_text_mat.msg" ":defaultShaderList1.s" -na
		;
connectAttr "place2dTexture25.msg" ":defaultRenderUtilityList1.u" -na;
connectAttr "place2dTexture26.msg" ":defaultRenderUtilityList1.u" -na;
connectAttr "place2dTexture41.msg" ":defaultRenderUtilityList1.u" -na;
connectAttr "defaultRenderLayer.msg" ":defaultRenderingList1.r" -na;
connectAttr "texcoord_checker_up_bc.msg" ":defaultTextureList1.tx" -na;
connectAttr "uv_grid_ao.msg" ":defaultTextureList1.tx" -na;
connectAttr "uv_grid_ao_opacity.msg" ":defaultTextureList1.tx" -na;
// End of multi_uv_simple.ma
