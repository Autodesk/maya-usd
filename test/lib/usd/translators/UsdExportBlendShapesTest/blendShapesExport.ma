//Maya ASCII 2020 scene
//Name: blendShapesExport.ma
//Last modified: Mon, Jan 11, 2021 10:01:56 AM
//Codeset: UTF-8
requires maya "2020";
requires "mtoa" "4.0.4.1";
requires "stereoCamera" "10.0";
requires "stereoCamera" "10.0";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2020";
fileInfo "version" "2020";
fileInfo "cutIdentifier" "202009141615-87c40af620";
fileInfo "osv" "Mac OS X 10.16";
fileInfo "UUID" "7BF656E0-1E4C-539B-30DC-7F812558CD9D";
fileInfo "vrayBuild" "4.30.02 e646822";
createNode transform -s -n "persp";
	rename -uid "2077CAC5-BF47-33B8-4402-70A6511A185B";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 20.763300040363571 19.129491846540319 11.688706809040287 ;
	setAttr ".r" -type "double3" -36.338352729700354 55.399999999998691 -8.4016634219489009e-15 ;
createNode camera -s -n "perspShape" -p "persp";
	rename -uid "8991EE25-C345-99C4-8B97-049999C7F7C3";
	setAttr -k off ".v" no;
	setAttr ".fl" 34.999999999999986;
	setAttr ".coi" 31.222709660303792;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".tp" -type "double3" -0.26489830017089844 0.12959772348403931 -1.4749370149796515 ;
	setAttr ".hc" -type "string" "viewSet -p %camera";
	setAttr ".ai_translator" -type "string" "perspective";
createNode transform -s -n "top";
	rename -uid "89A988B4-C042-8FDA-E5D2-53957F1ADCA8";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 1000.1 0 ;
	setAttr ".r" -type "double3" -89.999999999999986 0 0 ;
createNode camera -s -n "topShape" -p "top";
	rename -uid "DDB88EB8-7741-706C-139F-8A8EFF87419F";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "top";
	setAttr ".den" -type "string" "top_depth";
	setAttr ".man" -type "string" "top_mask";
	setAttr ".hc" -type "string" "viewSet -t %camera";
	setAttr ".o" yes;
	setAttr ".ai_translator" -type "string" "orthographic";
createNode transform -s -n "front";
	rename -uid "6B55995D-4045-AC8B-54E9-819DA4C34E3F";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 0 1000.1 ;
createNode camera -s -n "frontShape" -p "front";
	rename -uid "567FCCF7-7D4F-6F44-5D7B-94B3B899DE31";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "front";
	setAttr ".den" -type "string" "front_depth";
	setAttr ".man" -type "string" "front_mask";
	setAttr ".hc" -type "string" "viewSet -f %camera";
	setAttr ".o" yes;
	setAttr ".ai_translator" -type "string" "orthographic";
createNode transform -s -n "side";
	rename -uid "A1E5F942-7446-C7F4-808F-A7807B4DC063";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 1000.1 0 0 ;
	setAttr ".r" -type "double3" 0 89.999999999999986 0 ;
createNode camera -s -n "sideShape" -p "side";
	rename -uid "65ACF8C2-BD43-F011-609D-5981EDB28E4B";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "side";
	setAttr ".den" -type "string" "side_depth";
	setAttr ".man" -type "string" "side_mask";
	setAttr ".hc" -type "string" "viewSet -s %camera";
	setAttr ".o" yes;
	setAttr ".ai_translator" -type "string" "orthographic";
createNode transform -n "basic_cube_1_blendshape_no_anim";
	rename -uid "8A3082A0-8F4A-D688-9B75-3488CF33B3C7";
createNode transform -n "base" -p "basic_cube_1_blendshape_no_anim";
	rename -uid "84A47F03-1A45-56D9-F7FA-4AAD9B832E40";
createNode mesh -n "baseShape" -p "|basic_cube_1_blendshape_no_anim|base";
	rename -uid "6BF03D66-7E45-ABE4-787F-39B9501FB37A";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShape1Orig" -p "|basic_cube_1_blendshape_no_anim|base";
	rename -uid "E705F3A4-3C43-5350-9DE0-D3BB7A6428EB";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt" -p "basic_cube_1_blendshape_no_anim";
	rename -uid "AF3CFB8C-2346-0035-9B3E-3BBBC26CA146";
	setAttr ".t" -type "double3" 0 0 -3.5547359776138951 ;
createNode mesh -n "tgtShape" -p "|basic_cube_1_blendshape_no_anim|tgt";
	rename -uid "FAA5E968-C14D-9C55-FBBB-5E841CFA30A5";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.375 0.25 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 7 ".pt[0:6]" -type "float3"  -0.13434362 0.14127305 0.14774829 
		-0.0021889743 0.0023018818 0.0024073881 -0.57672817 0.60647583 0.63427365 -0.13434362 
		0.14127305 0.14774829 -0.13434362 0.14127305 0.14774829 -0.0021889743 0.0023018818 
		0.0024073881 -0.0021889743 0.0023018818 0.0024073881;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "basic_cube_2_inbetweens_no_anim";
	rename -uid "E7807700-9047-24EF-E72F-C69882E10EAB";
createNode transform -n "base" -p "basic_cube_2_inbetweens_no_anim";
	rename -uid "BA9E3DFC-FD4F-836E-043B-819F17AE0218";
createNode mesh -n "baseShape" -p "|basic_cube_2_inbetweens_no_anim|base";
	rename -uid "DCDA6A94-0B47-4749-4182-FFA330C73E73";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShape1Orig" -p "|basic_cube_2_inbetweens_no_anim|base";
	rename -uid "FB5E8452-124A-FC70-6730-E3A367EF1FF7";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgts" -p "basic_cube_2_inbetweens_no_anim";
	rename -uid "431CBAAC-D944-BE34-3CC1-B9BB0D618A82";
createNode transform -n "tgt1" -p "|basic_cube_2_inbetweens_no_anim|tgts";
	rename -uid "05661251-B54C-0F24-5922-78BC61888757";
	setAttr ".t" -type "double3" 0 0 -2.994296718151257 ;
createNode mesh -n "tgtShape1" -p "|basic_cube_2_inbetweens_no_anim|tgts|tgt1";
	rename -uid "1635EF21-2040-2F95-5D6A-C18B466ECB93";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.375 0.25 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".pt[2]" -type "float3"  -0.29964432 0.23007409 0.2268036;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt2" -p "|basic_cube_2_inbetweens_no_anim|tgts";
	rename -uid "2B641D8D-B044-5ACD-847A-28A3FC5A6500";
	setAttr ".t" -type "double3" 0 0 -5.5180580455051489 ;
createNode mesh -n "tgtShape2" -p "|basic_cube_2_inbetweens_no_anim|tgts|tgt2";
	rename -uid "29845D13-434A-225D-9B59-54B0037A97F9";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.75 0.375 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".pt[5]" -type "float3"  0.32545197 -0.1377936 -1.0177023;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt3" -p "|basic_cube_2_inbetweens_no_anim|tgts";
	rename -uid "7072748C-8146-754F-DAAB-1AB35A3D2D79";
	setAttr ".t" -type "double3" 0 0 -8.3729123649740576 ;
createNode mesh -n "tgtShape3" -p "|basic_cube_2_inbetweens_no_anim|tgts|tgt3";
	rename -uid "F15AF0DC-2B45-11B0-31D0-65BCDB63F916";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.75 0.375 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 2 ".pt[4:5]" -type "float3"  -0.062199187 0.14360872 -0.61249459 
		-0.012448186 0.13965988 -0.88584316;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "basic_cube_4_blendshapes_no_anim";
	rename -uid "F163B881-1740-D0A1-96B9-7DB3A6BDA340";
createNode transform -n "base" -p "basic_cube_4_blendshapes_no_anim";
	rename -uid "A0D083FB-6547-16D1-E359-A2AE173AD675";
createNode mesh -n "baseShape" -p "|basic_cube_4_blendshapes_no_anim|base";
	rename -uid "14242F05-6D41-7FAF-13B2-31B53719D561";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShapeOrig" -p "|basic_cube_4_blendshapes_no_anim|base";
	rename -uid "5B186737-B84C-8175-6B53-5D8C6C8928EA";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgts" -p "basic_cube_4_blendshapes_no_anim";
	rename -uid "22A96978-2D41-EDDD-400F-C582086A63A8";
createNode transform -n "tgt" -p "|basic_cube_4_blendshapes_no_anim|tgts";
	rename -uid "EDF2D7EE-4844-9327-87DC-75A1D17E7F62";
	setAttr ".t" -type "double3" 0 0 -3.2795481397167543 ;
createNode mesh -n "tgtShape" -p "|basic_cube_4_blendshapes_no_anim|tgts|tgt";
	rename -uid "F64AA876-4844-46F9-A904-6DB249294A59";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.5 0.25 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 2 ".pt[2:3]" -type "float3"  0 0 0.47292137 0 0 0.47292137;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt2" -p "|basic_cube_4_blendshapes_no_anim|tgts";
	rename -uid "4D2A5DC1-A542-76A9-562B-5EA1E2CA69D5";
	setAttr ".t" -type "double3" 0 0 -6.4393270034554639 ;
createNode mesh -n "tgt2Shape" -p "|basic_cube_4_blendshapes_no_anim|tgts|tgt2";
	rename -uid "45737704-644A-5CDA-ABBF-249A440E253D";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.75 0.5 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 2 ".pt";
	setAttr ".pt[1]" -type "float3" 0.85104573 0 0 ;
	setAttr ".pt[7]" -type "float3" 0.85104573 0 0 ;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt3" -p "|basic_cube_4_blendshapes_no_anim|tgts";
	rename -uid "A595739B-8642-6ABE-0648-4E872DA52E97";
	setAttr ".t" -type "double3" 0 0 -9.7688307897999103 ;
createNode mesh -n "tgt3Shape" -p "|basic_cube_4_blendshapes_no_anim|tgts|tgt3";
	rename -uid "CE884B55-4A4E-0BE9-39CD-96A162C69AF4";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.5 0.5 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 3 ".pt";
	setAttr ".pt[0]" -type "float3" -0.78172261 0 0 ;
	setAttr ".pt[6]" -type "float3" -0.78172261 0 0 ;
	setAttr ".pt[7]" -type "float3" -0.78172261 0 0 ;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "basic_cube_4_blendshape_inbetween_anim";
	rename -uid "A97CB52A-E743-6EB4-3CDF-B1AA6E726B4B";
createNode transform -n "base" -p "basic_cube_4_blendshape_inbetween_anim";
	rename -uid "998C16A4-744F-19A7-08A3-D68D983D6F3B";
createNode mesh -n "baseShape" -p "|basic_cube_4_blendshape_inbetween_anim|base";
	rename -uid "67F99C5E-7A44-42C8-6633-8880D1BA26E1";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShapeOrig" -p "|basic_cube_4_blendshape_inbetween_anim|base";
	rename -uid "7E74153F-AD44-1DC8-D8CF-AC95D80C4525";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShapeOrig1" -p "|basic_cube_4_blendshape_inbetween_anim|base";
	rename -uid "65E03D78-D24C-591E-AEF1-31B40B396F7A";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgts" -p "basic_cube_4_blendshape_inbetween_anim";
	rename -uid "42474D8D-9F4A-33F1-6EAD-D0858426BE06";
createNode transform -n "tgt" -p "|basic_cube_4_blendshape_inbetween_anim|tgts";
	rename -uid "358EA7DE-3841-E475-0644-758B932B41BC";
	setAttr ".t" -type "double3" 0 0 -3.2795481397167543 ;
createNode mesh -n "tgtShape" -p "|basic_cube_4_blendshape_inbetween_anim|tgts|tgt";
	rename -uid "B30239E6-B74E-CA8B-96F5-58A38A81D9B1";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.5 0.25 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 2 ".pt[2:3]" -type "float3"  0 0 0.47292137 0 0 0.47292137;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt2" -p "|basic_cube_4_blendshape_inbetween_anim|tgts";
	rename -uid "5EA4D23B-6F40-9EB0-B6B8-918D5AC6ADE5";
	setAttr ".t" -type "double3" 0 0 -6.4393270034554639 ;
createNode mesh -n "tgt2Shape" -p "|basic_cube_4_blendshape_inbetween_anim|tgts|tgt2";
	rename -uid "F64AA3DA-FA4E-A070-7604-FA98681F1291";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.75 0.5 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 2 ".pt";
	setAttr ".pt[1]" -type "float3" 0.85104573 0 0 ;
	setAttr ".pt[7]" -type "float3" 0.85104573 0 0 ;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt3" -p "|basic_cube_4_blendshape_inbetween_anim|tgts";
	rename -uid "B85F6B80-2D49-C87E-4778-6DAF35F76069";
	setAttr ".t" -type "double3" 0 0 -9.7688307897999103 ;
createNode mesh -n "tgt3Shape" -p "|basic_cube_4_blendshape_inbetween_anim|tgts|tgt3";
	rename -uid "B6073707-3B4C-8AB8-F196-56845A09B9AA";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.5 0.5 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 3 ".pt";
	setAttr ".pt[0]" -type "float3" -0.78172261 0 0 ;
	setAttr ".pt[6]" -type "float3" -0.78172261 0 0 ;
	setAttr ".pt[7]" -type "float3" -0.78172261 0 0 ;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "basic_cube_3_blendshape_anim";
	rename -uid "B3A329F8-E148-2281-06D0-16B6E7906DFF";
createNode transform -n "base" -p "basic_cube_3_blendshape_anim";
	rename -uid "EA7F2A89-1449-3F36-77CC-BE8E8574716B";
	setAttr ".t" -type "double3" 0 0 -3.5527136788005009e-15 ;
createNode mesh -n "baseShape" -p "|basic_cube_3_blendshape_anim|base";
	rename -uid "892433F7-474F-B981-2862-C0AB73E2B855";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShapeOrig" -p "|basic_cube_3_blendshape_anim|base";
	rename -uid "4FDACD3B-6845-C15C-6C3A-80836F8621FC";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShapeOrig2" -p "|basic_cube_3_blendshape_anim|base";
	rename -uid "7DFA97F2-1B49-C0CC-810E-BEAB7589F021";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt" -p "basic_cube_3_blendshape_anim";
	rename -uid "26DE1CB9-A149-38BF-EB03-529ADF4608BE";
	setAttr ".t" -type "double3" 0 0 -2.9498740299593065 ;
createNode mesh -n "tgtShape" -p "|basic_cube_3_blendshape_anim|tgt";
	rename -uid "27BE3B43-E744-FF83-ED0F-58AEECC412E6";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.375 0.25 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".pt[2]" -type "float3"  -0.52979666 0.25919548 0.79796624;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "basic_cube_2_inbetween_anim";
	rename -uid "5CB38451-CC4E-A48E-593A-E7A44C9AFC5E";
createNode transform -n "base" -p "basic_cube_2_inbetween_anim";
	rename -uid "E366B4F3-C844-0C59-EC0E-39BFD8DF9611";
createNode mesh -n "baseShape" -p "|basic_cube_2_inbetween_anim|base";
	rename -uid "7288CE89-3A4C-0BD5-3559-018968F4A294";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShape1Orig" -p "|basic_cube_2_inbetween_anim|base";
	rename -uid "F50DE5ED-EE4D-EF54-FC85-D1B77A1510DF";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShapeOrig" -p "|basic_cube_2_inbetween_anim|base";
	rename -uid "E638ABF1-0C49-EE5D-E10B-E2872F53C14A";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.49999997 -0.49999994 0.82545203 0.36220634 -1.51770234 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgts" -p "basic_cube_2_inbetween_anim";
	rename -uid "F3EC6DBE-3D42-BDB1-04F6-B29A1E74A4F5";
createNode transform -n "tgt1" -p "|basic_cube_2_inbetween_anim|tgts";
	rename -uid "92E61909-734E-9ED5-D997-8EBC8E546D4F";
	setAttr ".t" -type "double3" 0 0 -2.994296718151257 ;
createNode mesh -n "tgtShape1" -p "|basic_cube_2_inbetween_anim|tgts|tgt1";
	rename -uid "EBF644BB-5740-9057-9E81-658779C6163F";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.375 0.25 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".pt[2]" -type "float3"  -0.29964432 0.23007409 0.2268036;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt2" -p "|basic_cube_2_inbetween_anim|tgts";
	rename -uid "7A92F96B-404D-6D69-481D-6C92E652B3FF";
	setAttr ".t" -type "double3" 0 0 -5.5180580455051489 ;
createNode mesh -n "tgtShape2" -p "|basic_cube_2_inbetween_anim|tgts|tgt2";
	rename -uid "2B9DAC18-9442-CAFD-BCC8-23B99E8D5E8C";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.75 0.375 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".pt[5]" -type "float3"  0.32545197 -0.1377936 -1.0177023;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt3" -p "|basic_cube_2_inbetween_anim|tgts";
	rename -uid "438A3E64-FC48-6CC2-E814-A7AB8A444AAB";
	setAttr ".t" -type "double3" 0 0 -8.3729123649740576 ;
createNode mesh -n "tgtShape3" -p "|basic_cube_2_inbetween_anim|tgts|tgt3";
	rename -uid "D8076241-A342-28E9-A68B-7B95100EB641";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.75 0.375 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 2 ".pt[4:5]" -type "float3"  -0.062199187 0.14360872 -0.61249459 
		-0.012448186 0.13965988 -0.88584316;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "cube_double_blendshape_setup";
	rename -uid "E32D2E8F-4F40-0A56-1BAF-EAB38D09B7D9";
createNode transform -n "base" -p "cube_double_blendshape_setup";
	rename -uid "1679E6A7-F44B-FE7B-5862-C6B42072BA2C";
createNode transform -n "pCube1base" -p "|cube_double_blendshape_setup|base";
	rename -uid "69BAD580-4248-0F94-8E7A-469805073A84";
createNode mesh -n "pCube1baseShape" -p "pCube1base";
	rename -uid "C03E1FB0-D64F-339B-B2FA-81BD7E5A10BC";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "pCube1baseShape1Orig" -p "pCube1base";
	rename -uid "2540AEB6-9E4F-FEA2-A2C1-01BFA4227F76";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "pCube2base" -p "|cube_double_blendshape_setup|base";
	rename -uid "EAF44DF2-594E-248F-6CEE-FC8A9953D7C6";
	setAttr ".t" -type "double3" 0 0 -2.5634380260952963 ;
createNode mesh -n "pCube2baseShape" -p "pCube2base";
	rename -uid "30BCCE68-8140-FBEE-FBE4-B0A800D37A08";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "pCube2baseShape2Orig" -p "pCube2base";
	rename -uid "D27CCD37-BF42-25EB-A6AB-669AD4F55538";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgts" -p "cube_double_blendshape_setup";
	rename -uid "A7488CAB-9C4B-5436-2C4F-BD84978C32F5";
	setAttr ".t" -type "double3" 0 0 -5.1436453859959803 ;
createNode transform -n "pCube1tgt" -p "|cube_double_blendshape_setup|tgts";
	rename -uid "9F6FC77E-1E43-7C79-F0F7-D5BB99DE3198";
createNode mesh -n "pCube1tgtShape" -p "pCube1tgt";
	rename -uid "8E8738C1-5B4C-49EE-F764-B583E8858106";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.375 0.25 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".pt[2]" -type "float3"  -1.2726057 0.88935614 0.87216431;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "pCube2tgt" -p "|cube_double_blendshape_setup|tgts";
	rename -uid "14B6E4A7-5446-6AB3-F2EE-179D693BB6B7";
	setAttr ".t" -type "double3" 0 0 -2.9275321921911726 ;
createNode mesh -n "pCube2tgtShape" -p "pCube2tgt";
	rename -uid "B67C6297-3F4E-ACFE-5853-8BB40C0CE3D5";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.25 0.375 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".pt[4]" -type "float3"  0.079416439 1.2315221 -1.7167541;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "cube_extra_deformer_in_stack";
	rename -uid "BF8BD194-614E-778F-BD68-B18EA1F55928";
createNode transform -n "group9" -p "cube_extra_deformer_in_stack";
	rename -uid "F761C31A-D248-5BA3-7F80-6980649F7DE2";
createNode transform -n "base" -p "group9";
	rename -uid "AE2249CA-F545-F5F5-2EB7-549B4535C30A";
createNode mesh -n "baseShape" -p "|cube_extra_deformer_in_stack|group9|base";
	rename -uid "45F30D9F-CA43-83FC-A2C7-47A1F1F0BF2B";
	setAttr -k off ".v";
	setAttr -s 6 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShapeOrig" -p "|cube_extra_deformer_in_stack|group9|base";
	rename -uid "F6E69A3F-2E41-D426-8CCC-77B0D42A6817";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt" -p "group9";
	rename -uid "4408F8FF-3045-EFC5-8E5C-C78F21FF2193";
	setAttr ".t" -type "double3" 0 0 -3.4082148735620685 ;
createNode mesh -n "tgtShape" -p "|cube_extra_deformer_in_stack|group9|tgt";
	rename -uid "EC6FA016-6842-5EF6-817D-A39AACCB8044";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.375 0.25 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".pt[2]" -type "float3"  -1.0112363 0.79718959 1.210837;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "squash1Handle";
	rename -uid "FF53A829-344D-4B71-F6C3-22AE936BB0CA";
	setAttr ".t" -type "double3" -26.979134824314716 0 0 ;
	setAttr ".s" -type "double3" 0.5 0.5 0.5 ;
	setAttr ".smd" 7;
createNode deformSquash -n "squash1HandleShape" -p "squash1Handle";
	rename -uid "58E268E2-B344-1086-7B41-268269270719";
	setAttr -k off ".v";
	setAttr ".dd" -type "doubleArray" 7 -1 1 0 0 0.5 1 0 ;
	setAttr ".hw" 0.55;
createNode transform -n "basic_cube_4_blendshape_baked_targets";
	rename -uid "5B454CEB-3D4A-5447-B147-DCB1C7022110";
createNode transform -n "base" -p "basic_cube_4_blendshape_baked_targets";
	rename -uid "674932A4-F545-BF53-E109-6FA4ED6B5EB5";
createNode mesh -n "baseShape" -p "|basic_cube_4_blendshape_baked_targets|base";
	rename -uid "1FDC20E4-0B43-2D0E-5B08-D1BCF2CC83FD";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShapeOrig" -p "|basic_cube_4_blendshape_baked_targets|base";
	rename -uid "F985112C-E743-A757-62DC-17B9F2ED7AE7";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgts" -p "basic_cube_4_blendshape_baked_targets";
	rename -uid "8F8F6070-F74C-19A4-6E0F-D3A5AF61503A";
createNode transform -n "tgt" -p "|basic_cube_4_blendshape_baked_targets|tgts";
	rename -uid "21C7630D-3E4D-179D-4600-1494341477AA";
	setAttr ".t" -type "double3" 0 0 -3.2795481397167543 ;
createNode mesh -n "tgtShape" -p "|basic_cube_4_blendshape_baked_targets|tgts|tgt";
	rename -uid "92EC1490-904C-64EB-B76D-298D3335D861";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.5 0.25 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 2 ".pt[2:3]" -type "float3"  0 0 0.47292137 0 0 0.47292137;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt2" -p "|basic_cube_4_blendshape_baked_targets|tgts";
	rename -uid "23EF151A-6B4F-044B-BFCB-54B876AE6D85";
	setAttr ".t" -type "double3" 0 0 -6.4393270034554639 ;
createNode mesh -n "tgt2Shape" -p "|basic_cube_4_blendshape_baked_targets|tgts|tgt2";
	rename -uid "A99D766A-3347-8BB1-C193-9CA35E8BBC6A";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.75 0.5 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 2 ".pt";
	setAttr ".pt[1]" -type "float3" 0.85104573 0 0 ;
	setAttr ".pt[7]" -type "float3" 0.85104573 0 0 ;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "tgt3" -p "|basic_cube_4_blendshape_baked_targets|tgts";
	rename -uid "B55E26CF-4742-54B6-A87C-03996DAABD66";
	setAttr ".t" -type "double3" 0 0 -9.7688307897999103 ;
createNode mesh -n "tgt3Shape" -p "|basic_cube_4_blendshape_baked_targets|tgts|tgt3";
	rename -uid "6B3C4C0C-4D46-DF2A-10FF-6493912EADF6";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.5 0.5 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 3 ".pt";
	setAttr ".pt[0]" -type "float3" -0.78172261 0 0 ;
	setAttr ".pt[6]" -type "float3" -0.78172261 0 0 ;
	setAttr ".pt[7]" -type "float3" -0.78172261 0 0 ;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".din" 33554432;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "basic_skinned_cube_blendshape_baked_target_anim";
	rename -uid "7EE29553-B64F-1D42-FC0D-90A182F4E7BF";
createNode transform -n "base" -p "basic_skinned_cube_blendshape_baked_target_anim";
	rename -uid "6167D462-A440-1379-E57E-518589EBD0B7";
	setAttr -l on ".tx";
	setAttr -l on ".ty";
	setAttr -l on ".tz";
	setAttr -l on ".rx";
	setAttr -l on ".ry";
	setAttr -l on ".rz";
	setAttr -l on ".sx";
	setAttr -l on ".sy";
	setAttr -l on ".sz";
createNode mesh -n "baseShape" -p "|basic_skinned_cube_blendshape_baked_target_anim|base";
	rename -uid "0E633C19-124B-5A26-0BD9-25B1404E6193";
	setAttr -k off ".v";
	setAttr -s 6 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".vcs" 2;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode mesh -n "baseShapeOrig" -p "|basic_skinned_cube_blendshape_baked_target_anim|base";
	rename -uid "207D1CD6-C24F-C3FD-662B-C9BBC347CA7A";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode joint -n "joint2" -p "basic_skinned_cube_blendshape_baked_target_anim";
	rename -uid "6BB7300E-A547-AE37-064E-E5BF40692EE2";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".uoc" 1;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 0 89.999999999999986 0 ;
	setAttr ".bps" -type "matrix" 2.2204460492503131e-16 0 -1.0000000000000002 0 0 1 0 0
		 1.0000000000000002 0 2.2204460492503131e-16 0 -6 0 1 1;
	setAttr ".radi" 0.55172413793103448;
createNode joint -n "joint3" -p "joint2";
	rename -uid "2EED0681-1146-9EF1-691C-F9B20F965244";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".uoc" 1;
	setAttr ".oc" 1;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 0 -89.999999999999986 0 ;
	setAttr ".bps" -type "matrix" 1.0000000000000004 0 0 0 0 1 0 0 0 0 1.0000000000000004 0
		 -6 0 -1 1;
	setAttr ".radi" 0.55172413793103448;
createNode lightLinker -s -n "lightLinker1";
	rename -uid "46456D72-0E43-B636-1DC2-89982C9A3E5C";
	setAttr -s 2 ".lnk";
	setAttr -s 2 ".slnk";
createNode shapeEditorManager -n "shapeEditorManager";
	rename -uid "EFF3D16D-9645-6CD1-546A-95A43D14A242";
	setAttr ".bsdt[0].bscd" -type "Int32Array" 11 8 9 11 12 13
		 15 16 17 18 21 22 ;
	setAttr -s 11 ".bspr";
	setAttr -s 11 ".obsv";
createNode poseInterpolatorManager -n "poseInterpolatorManager";
	rename -uid "E5C73CF6-F44C-696F-E453-B697612F2B0E";
createNode displayLayerManager -n "layerManager";
	rename -uid "F903DABD-9C43-C446-5DD4-909C5014FA94";
createNode displayLayer -n "defaultLayer";
	rename -uid "66B28721-5A42-036C-3710-C7AE0952167E";
createNode renderLayerManager -n "renderLayerManager";
	rename -uid "F275558B-C34C-83BA-D6A3-3196243ED040";
createNode renderLayer -n "defaultRenderLayer";
	rename -uid "029602C9-1546-B736-B15A-E786C1A212A0";
	setAttr ".g" yes;
createNode script -n "sceneConfigurationScriptNode";
	rename -uid "5183990E-1B4C-D596-006D-BA8EA08B7515";
	setAttr ".b" -type "string" "playbackOptions -min 1 -max 120 -ast 1 -aet 200 ";
	setAttr ".st" 6;
createNode polyCube -n "polyCube1";
	rename -uid "522B89FF-954B-6751-354C-3489ED5EDAB2";
	setAttr ".cuv" 4;
createNode blendShape -n "blendShape9";
	rename -uid "4F24E935-7742-A0E2-3705-4C906020726A";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr ".w[0]"  0;
	setAttr ".mlid" 8;
	setAttr ".mlpr" 0;
	setAttr ".pndr[0]"  0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 1 0 ;
	setAttr ".aal" -type "attributeAlias" {"pCube2","weight[0]"} ;
createNode tweak -n "tweak9";
	rename -uid "D752EEB0-414C-4E9C-ECB2-4FAB7337A865";
createNode objectSet -n "blendShape9Set";
	rename -uid "FA356978-4244-90F4-86D7-B6A5FD6D0EAC";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape9GroupId";
	rename -uid "1E137720-B14B-3BF8-C969-BDB890D8B093";
	setAttr ".ihi" 0;
createNode groupParts -n "blendShape9GroupParts";
	rename -uid "759AF97A-194E-DCA6-E6AB-3F81437DFF35";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet9";
	rename -uid "F14ABF3A-BA42-C0D2-F262-988C00481D7D";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId18";
	rename -uid "EB0CE420-934B-B3E7-D732-0A9847EFFBE0";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts18";
	rename -uid "7E68F37E-8843-51E5-0FEE-A0AE630D25A6";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode polyCube -n "polyCube2";
	rename -uid "3E04ED27-9C4F-3FC2-C38B-859C593253A9";
	setAttr ".cuv" 4;
createNode blendShape -n "blendShape10";
	rename -uid "FFBA128F-EA4D-6556-A365-5E8AA36C1E8D";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr ".w[0]"  0;
	setAttr -s 3 ".it[0].itg[0].iti";
	setAttr ".mlid" 9;
	setAttr ".mlpr" 0;
	setAttr ".pndr[0]"  0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 1 0 ;
	setAttr -s 2 ".ibig[0].ibi";
	setAttr ".ibig[0].ibi[5333].ibtn" -type "string" "pCube2_0.333";
	setAttr -s 4 ".ibig[0].ibi[5333].itc[0:3]" -type "float2" 0 0 0.2
		 0.2 0.5 0.5 1 1;
	setAttr ".ibig[0].ibi[5667].ibtn" -type "string" "pCube2_0.667";
	setAttr -s 4 ".ibig[0].ibi[5667].itc[0:3]" -type "float2" 0 0 0.2
		 0.2 0.5 0.5 1 1;
	setAttr ".aal" -type "attributeAlias" {"pCube2","weight[0]"} ;
createNode tweak -n "tweak10";
	rename -uid "056CE75F-4C4D-53CE-65AE-D59658F96EAD";
createNode objectSet -n "blendShape10Set";
	rename -uid "6AD96A78-EF46-DD23-7378-5AAB53FE5213";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape10GroupId";
	rename -uid "703FAB8C-8B49-5653-3029-618C3126CA96";
	setAttr ".ihi" 0;
createNode groupParts -n "blendShape10GroupParts";
	rename -uid "990233D5-4D42-F80B-32D6-7E96EA333DCF";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet10";
	rename -uid "1F490524-274F-8368-C386-B49F47628F2D";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId20";
	rename -uid "690DE3D1-E641-EC36-EF97-B49F3574D831";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts20";
	rename -uid "E5C6F98D-B941-234B-6030-2693EB9CC1E0";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode polyCube -n "polyCube4";
	rename -uid "1270AB59-3B4B-8087-D654-7BBD973CB46F";
	setAttr ".cuv" 4;
createNode blendShape -n "blendShape13";
	rename -uid "9ECC3D9E-4845-1F72-3692-20BE2A1B3CD6";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr -s 3 ".it[0].itg[0].iti";
	setAttr ".mlid" 12;
	setAttr ".mlpr" 0;
	setAttr ".pndr[0]"  0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 1 0 ;
	setAttr -s 2 ".ibig[0].ibi";
	setAttr ".ibig[0].ibi[5333].ibtn" -type "string" "tgt_0.333";
	setAttr -s 4 ".ibig[0].ibi[5333].itc[0:3]" -type "float2" 0 0 0.2
		 0.2 0.5 0.5 1 1;
	setAttr ".ibig[0].ibi[5667].ibtn" -type "string" "tgt_0.667";
	setAttr -s 4 ".ibig[0].ibi[5667].itc[0:3]" -type "float2" 0 0 0.2
		 0.2 0.5 0.5 1 1;
	setAttr ".aal" -type "attributeAlias" {"tgt","weight[0]"} ;
createNode tweak -n "tweak13";
	rename -uid "9B022B2A-F345-1ED3-70B0-E6A7ED4CB321";
createNode objectSet -n "blendShape13Set";
	rename -uid "82429D6D-AC42-C2DD-DB0C-64A7411A5533";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape13GroupId";
	rename -uid "58B9107A-2F4D-F844-18B7-6599B52C1DA9";
	setAttr ".ihi" 0;
createNode groupParts -n "blendShape13GroupParts";
	rename -uid "81061C6D-0A4F-22DC-C506-50A1B11F4CFA";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet13";
	rename -uid "F0683E73-AC47-231A-1061-96B3FD389627";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId26";
	rename -uid "6FC48F97-2441-8FF3-D868-E981D791ACE9";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts26";
	rename -uid "22FCB46B-3247-BAA6-B8EF-A2BC17FC7849";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode blendShape -n "blendShape14";
	rename -uid "7FB17421-1740-ECE0-7BDC-1B8BD2D80571";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr -s 3 ".w";
	setAttr -s 3 ".w";
	setAttr -s 3 ".it[0].itg";
	setAttr ".mlid" 13;
	setAttr ".mlpr" 0;
	setAttr -s 3 ".pndr[0:2]"  0 0 0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 3 0 1 2 ;
	setAttr ".aal" -type "attributeAlias" {"tgt3","weight[0]","tgt2","weight[1]","tgt1"
		,"weight[2]"} ;
createNode tweak -n "tweak14";
	rename -uid "06D21109-E547-1A7D-94E9-979249A2349A";
createNode objectSet -n "blendShape14Set";
	rename -uid "F3F61F49-664E-7FBE-5908-6DB64D1535E3";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape14GroupId";
	rename -uid "17232796-8E49-03EF-228E-59AF53E784BE";
	setAttr ".ihi" 0;
createNode groupParts -n "blendShape14GroupParts";
	rename -uid "6F24A704-B84D-28F1-1F6E-F6B1F3C3F1C8";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet14";
	rename -uid "83421C0E-084C-DD84-9B8C-0E887735B0E2";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId28";
	rename -uid "271B4350-6E4E-4C50-E064-A6A4DC85A662";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts28";
	rename -uid "D86BAE70-0A46-1B28-0416-AF993634F548";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode blendShape -n "blendShape16";
	rename -uid "D4C74460-3843-E4B0-2FFA-498E39EF52C6";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr ".mlid" 15;
	setAttr ".mlpr" 0;
	setAttr ".pndr[0]"  0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 1 0 ;
	setAttr ".aal" -type "attributeAlias" {"tgt","weight[0]"} ;
createNode tweak -n "tweak16";
	rename -uid "680E81E1-9D49-CF15-952C-ABAA2C27444A";
createNode objectSet -n "blendShape16Set";
	rename -uid "7328D706-D144-A7FD-5640-E68011ECDC02";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape16GroupId";
	rename -uid "93D68BEA-1A42-4580-7343-D6BB14256B01";
	setAttr ".ihi" 0;
createNode groupParts -n "blendShape16GroupParts";
	rename -uid "FE1FE59C-6646-6413-C72C-40BA54C78227";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet16";
	rename -uid "A07668DA-074E-238D-9529-BAB9EBB7ABE4";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId32";
	rename -uid "E23C072A-784B-240A-F772-AEB5F124A419";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts32";
	rename -uid "DC67E14B-7341-97D1-2199-948F2BABA692";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode animCurveTU -n "blendShape16_envelope";
	rename -uid "C82F3321-C544-9564-EB8B-7E9FA5501191";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 14.988698979591836 1;
createNode animCurveTU -n "blendShape16_tgt";
	rename -uid "3C90CD0D-FB40-698D-D41D-0EBF457BD36A";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 14.988698979591836 1;
createNode animCurveTU -n "blendShape16_targetDirectory_0__directoryWeight";
	rename -uid "2223DFA9-EA4D-380F-7E7E-56BE0B82F4E0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 14.988698979591836 1;
createNode animCurveTU -n "blendShape13_tgt";
	rename -uid "F3F351E0-F24A-966F-10CA-42BA1ABEB675";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 13.966225510204081 1;
createNode animCurveTU -n "blendShape14_tgt3";
	rename -uid "FE19537C-5A4F-2694-638B-E3AC9F1D96FF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 9.7687034013605434 1;
createNode animCurveTU -n "blendShape14_tgt2";
	rename -uid "BFB3D705-EC40-F3DF-3DD1-50A55593B7E6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 19.024777891156461 1;
createNode animCurveTU -n "blendShape14_tgt1";
	rename -uid "54B6310E-3B40-FF78-099E-F6AD4DED4322";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 28.388481292517007 1;
createNode polyCube -n "polyCube5";
	rename -uid "9E899288-A54A-D4A2-0E01-BE9D82F2A5E2";
	setAttr ".cuv" 4;
createNode blendShape -n "blendShape17";
	rename -uid "00878C5E-6048-4E60-2A63-38A39AD87898";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr ".w[0]"  0;
	setAttr ".mlid" 16;
	setAttr ".mlpr" 0;
	setAttr ".pndr[0]"  0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 1 0 ;
	setAttr ".aal" -type "attributeAlias" {"pCube1","weight[0]"} ;
createNode tweak -n "tweak17";
	rename -uid "BE6F43A4-164B-F10E-9EA5-52838A62EF2C";
createNode objectSet -n "blendShape17Set";
	rename -uid "45E13EF7-6144-1962-D08B-CA80693B20E1";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape17GroupId";
	rename -uid "8E450C23-CA4F-53D0-A771-99894560D219";
	setAttr ".ihi" 0;
createNode groupParts -n "blendShape17GroupParts";
	rename -uid "87760144-344B-0C3D-6F27-E8AAC1DFA1A8";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet17";
	rename -uid "AA762A54-1C48-BD8F-8684-35802BCC5A0E";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId34";
	rename -uid "CD99FE39-374B-722C-3DFD-62805CAC68AE";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts34";
	rename -uid "6483CD86-C643-0A3D-0D4A-C29A40D01064";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode blendShape -n "blendShape18";
	rename -uid "FD5C9F03-7D43-EB2B-E038-4F8CCCC5A9FC";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr ".mlid" 17;
	setAttr ".mlpr" 0;
	setAttr ".pndr[0]"  0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 1 0 ;
	setAttr ".aal" -type "attributeAlias" {"pCube2","weight[0]"} ;
createNode tweak -n "tweak18";
	rename -uid "97ACA189-8741-790C-E2CB-EAA8208DF7A9";
createNode objectSet -n "blendShape18Set";
	rename -uid "11FB00EA-D74D-4BCF-6D24-0883DA628F63";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape18GroupId";
	rename -uid "B12A87E9-C840-72B5-73AD-E080DABF062D";
	setAttr ".ihi" 0;
createNode groupParts -n "blendShape18GroupParts";
	rename -uid "380D9F02-194F-E09E-499E-30920736D634";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet18";
	rename -uid "CEE01092-5E47-11E1-9668-FEBB1907524E";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId36";
	rename -uid "E2EC3F7F-8F47-8610-715E-B983AA5A9226";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts36";
	rename -uid "472B6B67-184A-BD6D-4B69-139ACECA8A6E";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode polyCube -n "polyCube6";
	rename -uid "FB239143-EC47-DA91-492B-65A8682CE912";
	setAttr ".cuv" 4;
createNode nonLinear -n "squash1";
	rename -uid "EDA449B2-6F47-8526-B058-999E2C107A93";
	addAttr -uap -is true -ci true -k true -sn "fac" -ln "factor" -smn -10 -smx 10 
		-at "double";
	addAttr -uap -is true -ci true -k true -sn "exp" -ln "expand" -dv 1 -min 0 -smn 
		0 -smx 10 -at "double";
	addAttr -uap -is true -ci true -k true -sn "mp" -ln "maxExpandPos" -dv 0.5 -min 
		0.01 -max 0.99 -at "double";
	addAttr -uap -is true -ci true -k true -sn "ss" -ln "startSmoothness" -min 0 -max 
		1 -at "double";
	addAttr -uap -is true -ci true -k true -sn "es" -ln "endSmoothness" -min 0 -max 
		1 -at "double";
	addAttr -uap -is true -ci true -k true -sn "lb" -ln "lowBound" -dv -1 -max 0 -smn 
		-10 -smx 0 -at "double";
	addAttr -uap -is true -ci true -k true -sn "hb" -ln "highBound" -dv 1 -min 0 -smn 
		0 -smx 10 -at "double";
	setAttr -av -k on ".fac";
	setAttr -av -k on ".exp";
	setAttr -av -k on ".mp";
	setAttr -av -k on ".ss";
	setAttr -av -k on ".es";
	setAttr -av -k on ".lb";
	setAttr -av -k on ".hb";
createNode tweak -n "tweak19";
	rename -uid "DE0F7BF1-814E-2FB7-78E1-D7AA6F6F4610";
createNode objectSet -n "squash1Set";
	rename -uid "77B405A4-354B-9C1F-B97E-5AAAE6F03E14";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "squash1GroupId";
	rename -uid "6A3FE307-1740-2431-2998-E898307838C2";
	setAttr ".ihi" 0;
createNode groupParts -n "squash1GroupParts";
	rename -uid "198BCFEE-1E4A-3A85-2FB3-5891BD0479C8";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet19";
	rename -uid "2D1D5313-6943-8749-7CC0-4E999273F910";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId38";
	rename -uid "87E82CFA-7145-79C1-1B28-8C9153020F13";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts38";
	rename -uid "CAB2CBD7-F54B-3A50-37E5-47B3C3D3EB50";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode blendShape -n "blendShape19";
	rename -uid "DB80BAE9-504F-FD98-0B08-C285FD0C25EE";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr ".w[0]"  0;
	setAttr ".mlid" 18;
	setAttr ".mlpr" 0;
	setAttr ".pndr[0]"  0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 1 0 ;
	setAttr ".dfo" 1;
	setAttr ".aal" -type "attributeAlias" {"tgt","weight[0]"} ;
createNode objectSet -n "blendShape19Set";
	rename -uid "54E3542E-404F-747E-187F-E0845467F16A";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape19GroupId";
	rename -uid "82907C8A-F94B-9438-92C3-FE8E48EE1D28";
	setAttr ".ihi" 0;
createNode groupParts -n "blendShape19GroupParts";
	rename -uid "2197A777-6B49-878F-202F-5EBE0093BBEC";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode blendShape -n "blendShape12";
	rename -uid "9C1A8AAC-E64F-2963-DA3D-74AD41AA0EF4";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr -s 3 ".w[0:2]"  0 0 0;
	setAttr -s 3 ".it[0].itg";
	setAttr ".mlid" 11;
	setAttr ".mlpr" 0;
	setAttr -s 3 ".pndr[0:2]"  0 0 0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 3 0 1 2 ;
	setAttr ".aal" -type "attributeAlias" {"tgt3","weight[0]","tgt2","weight[1]","tgt"
		,"weight[2]"} ;
createNode objectSet -n "blendShape12Set";
	rename -uid "58F62D6D-6E45-A2BE-E03E-99952C150410";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape12GroupId";
	rename -uid "1DB29C61-9B4E-3788-925A-9B8BB882C054";
	setAttr ".ihi" 0;
createNode groupParts -n "blendShape12GroupParts";
	rename -uid "3B6CD3E2-D14C-C189-381B-B0B02CF5DB98";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode tweak -n "tweak12";
	rename -uid "212333EE-0141-06FF-EAE9-8D932E9BABA5";
createNode objectSet -n "tweakSet12";
	rename -uid "F6FB1149-E34D-30C5-9E36-DD963FFFC58E";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId24";
	rename -uid "597D0A8C-5E47-8880-FB33-FE9D81976704";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts24";
	rename -uid "EA70B124-C749-02BA-A7E9-6A882C21A22F";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "blendShape12Set1";
	rename -uid "7C20B503-E34B-8001-BBEF-D1B39A8B6078";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape12GroupId1";
	rename -uid "4E7731EE-C042-61B3-09EC-B2AA3BF90FA6";
	setAttr ".ihi" 0;
createNode blendShape -n "blendShape22";
	rename -uid "4908818C-744E-EC70-E572-66A86208530E";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr -s 3 ".w[0:2]"  1 0 0;
	setAttr -s 3 ".it[0].itg";
	setAttr ".it[0].itg[0].iti[6000].ipt" -type "pointArray" 3 -0.7817225456237793
		 0 0 1 -0.7817225456237793 0 0 1 -0.78172260522842407 0 0 1 ;
	setAttr ".it[0].itg[0].iti[6000].ict" -type "componentList" 2 "vtx[0]" "vtx[6:7]";
	setAttr ".it[0].itg[1].iti[6000].ipt" -type "pointArray" 2 0.85104572772979736
		 0 0 1 0.85104572772979736 0 0 1 ;
	setAttr ".it[0].itg[1].iti[6000].ict" -type "componentList" 2 "vtx[1]" "vtx[7]";
	setAttr ".it[0].itg[2].iti[6000].ipt" -type "pointArray" 2 0 0 0.47292137145996094
		 1 0 0 0.47292137145996094 1 ;
	setAttr ".it[0].itg[2].iti[6000].ict" -type "componentList" 1 "vtx[2:3]";
	setAttr ".mlid" 21;
	setAttr ".mlpr" 0;
	setAttr -s 3 ".pndr[0:2]"  0 0 0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 3 0 1 2 ;
	setAttr ".aal" -type "attributeAlias" {"tgt3","weight[0]","tgt2","weight[1]","tgt"
		,"weight[2]"} ;
createNode groupParts -n "blendShape12GroupParts1";
	rename -uid "53AD3DEC-734A-C30E-0E95-0591CD248CA0";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode tweak -n "tweak22";
	rename -uid "17F7B8A2-3745-F3A6-7AA0-678895C228A8";
createNode objectSet -n "tweakSet22";
	rename -uid "8E8F7C6D-5E49-8FD2-A588-1D8B8D9C9392";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId43";
	rename -uid "C8175A27-AB48-396F-76C3-3C86B1661448";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts43";
	rename -uid "B2F308A0-D04F-F4A0-D4A3-B08839086B21";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode polyCube -n "polyCube7";
	rename -uid "95137DE4-F945-1A7D-814E-CDB0BE78CE19";
	setAttr ".cuv" 4;
createNode skinCluster -n "skinCluster2";
	rename -uid "677463F8-D84C-9A63-AC13-27B4A4BA45D4";
	setAttr -s 8 ".wl";
	setAttr ".wl[0:7].w"
		2 0 0.85712288128259462 1 0.14287711871740547
		2 0 0.9886539866732359 1 0.011346013326764206
		2 0 0.85712288128259462 1 0.14287711871740547
		2 0 0.9886539866732359 1 0.011346013326764206
		2 0 0.57410486203748967 1 0.42589513796251027
		2 0 0.78767572210252079 1 0.21232427789747929
		2 0 0.57410486203748967 1 0.42589513796251027
		2 0 0.78767572210252079 1 0.21232427789747929;
	setAttr -s 2 ".pm";
	setAttr ".pm[0]" -type "matrix" 2.2204460492503121e-16 0 0.99999999999999978 0 0 1 0 0
		 -0.99999999999999978 0 2.2204460492503121e-16 0 1.0000000000000011 0 5.9999999999999991 1;
	setAttr ".pm[1]" -type "matrix" 0.99999999999999956 0 0 0 0 1 0 0 0 0 0.99999999999999956 0
		 5.9999999999999973 0 0.99999999999999956 1;
	setAttr ".gm" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 -6.6412643691244098 0 0 1;
	setAttr -s 2 ".ma";
	setAttr -s 2 ".dpf[0:1]"  4 4;
	setAttr -s 2 ".lw";
	setAttr -s 2 ".lw";
	setAttr ".mmi" yes;
	setAttr ".mi" 5;
	setAttr ".ucm" yes;
	setAttr -s 2 ".ifcl";
	setAttr -s 2 ".ifcl";
createNode tweak -n "tweak23";
	rename -uid "AF283811-CB4F-DF4B-2BA8-25B2DF03999F";
createNode objectSet -n "skinCluster2Set";
	rename -uid "DD541541-8E40-C0AC-0496-5BB058D7A7A5";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "skinCluster2GroupId";
	rename -uid "ACB74984-F04B-AEE1-E94A-45BCBBCC8921";
	setAttr ".ihi" 0;
createNode groupParts -n "skinCluster2GroupParts";
	rename -uid "3B106355-EC41-BE5E-0E44-AFBEC9DC9573";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet23";
	rename -uid "535E85F1-DC40-2EC4-BA09-C7A33885D76E";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId45";
	rename -uid "F62B2377-CB48-E178-EEC5-9F824E5E6E3D";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts45";
	rename -uid "12F1C3B4-314B-5EAC-94B2-30803619B29D";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode dagPose -n "bindPose2";
	rename -uid "EB216000-1647-308A-2BD1-28A1A91A843C";
	setAttr -s 3 ".wm";
	setAttr ".wm[0]" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 -6.6412643691244098 0 0 1;
	setAttr -s 3 ".xm";
	setAttr ".xm[0]" -type "matrix" "xform" 1 1 1 0 0 0 0 -6.6412643691244098 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1 1 1 1 yes;
	setAttr ".xm[1]" -type "matrix" "xform" 1 1 1 0 0 0 0 0.64126436912440976 0
		 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0.70710678118654746 0 0.70710678118654768 1
		 1 1 yes;
	setAttr ".xm[2]" -type "matrix" "xform" 1 1 1 0 0 0 0 1.9999999999999996 0 -4.4408920985006242e-16 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 -0.70710678118654746 0 0.70710678118654768 1
		 1 1 yes;
	setAttr -s 3 ".m";
	setAttr -s 3 ".p";
	setAttr -s 3 ".g[0:2]" yes no no;
	setAttr ".bp" yes;
createNode animCurveTU -n "joint3_visibility";
	rename -uid "07410E68-6D4A-FE58-4100-8DB400D1BF4B";
	setAttr ".tan" 9;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 20 1;
	setAttr -s 2 ".kot[0:1]"  5 5;
createNode animCurveTL -n "joint3_translateX";
	rename -uid "42DE8C41-D54C-0810-E5FE-6097FA2C5555";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1.9999999999999996 20 4.2522203846771003;
createNode animCurveTL -n "joint3_translateY";
	rename -uid "68E32F5F-364F-2229-364A-5CAC65704133";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 20 0;
createNode animCurveTL -n "joint3_translateZ";
	rename -uid "7D945650-9844-7BB0-BC1E-97BE6A7211C2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 -4.4408920985006242e-16 20 -9.4418259536979139e-16;
createNode animCurveTA -n "joint3_rotateX";
	rename -uid "56083EF6-0844-2E0C-EDDB-1CAA760B97BD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 20 0;
createNode animCurveTA -n "joint3_rotateY";
	rename -uid "65280A93-804A-7A13-718B-78AE7FFC37B0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 20 0;
createNode animCurveTA -n "joint3_rotateZ";
	rename -uid "617A769F-854A-8E37-E049-909B91D147CE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 20 0;
createNode animCurveTU -n "joint3_scaleX";
	rename -uid "4CB1E15B-EC46-3687-327C-F584181B63FB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 20 1;
createNode animCurveTU -n "joint3_scaleY";
	rename -uid "5C3F4D8C-1F4D-5E98-81FA-4D83D5725504";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 20 1;
createNode animCurveTU -n "joint3_scaleZ";
	rename -uid "D070C35F-C44B-B648-1014-CCA646AB72FF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 20 1;
createNode blendShape -n "blendShape23";
	rename -uid "B5F3565D-CD4A-21C9-C047-A780108A8DE2";
	addAttr -ci true -h true -sn "aal" -ln "attributeAliasList" -dt "attributeAlias";
	setAttr ".w[0]"  0;
	setAttr -av ".w[0]";
	setAttr ".it[0].itg[0].iti[6000].ipt" -type "pointArray" 1 -0.70974814891815186
		 0.56029319763183594 0.61569833755493164 1 ;
	setAttr ".it[0].itg[0].iti[6000].ict" -type "componentList" 1 "vtx[2]";
	setAttr ".mlid" 22;
	setAttr ".mlpr" 0;
	setAttr ".pndr[0]"  0;
	setAttr ".tgdt[0].cid" -type "Int32Array" 1 0 ;
	setAttr ".aal" -type "attributeAlias" {"tgt","weight[0]"} ;
createNode objectSet -n "blendShape23Set";
	rename -uid "60258D62-174E-37C2-6880-52ACA7068D71";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "blendShape23GroupId";
	rename -uid "8E24E102-564A-5ECA-3D41-E68A08E533D7";
	setAttr ".ihi" 0;
createNode groupParts -n "blendShape23GroupParts";
	rename -uid "EEF6FCE3-EA47-40B8-5A0B-518D8765F783";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode animCurveTU -n "blendShape23_tgt";
	rename -uid "F76A53F9-3247-B391-D8DC-A0A989651775";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 20 1;
createNode animCurveTA -n "joint2_rotateX";
	rename -uid "69C8C90B-3245-F945-8271-55850B6CDEC5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 20 0;
createNode animCurveTA -n "joint2_rotateY";
	rename -uid "05F11EDC-144B-0D7D-532D-4E83D53C8B1C";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 20 0;
createNode animCurveTA -n "joint2_rotateZ";
	rename -uid "4B9B2BFA-D84B-9F35-BFAF-C69887213565";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 20 69.920498253116563;
createNode animCurveTU -n "joint2_visibility";
	rename -uid "B82045D3-2440-A89F-481D-AFA9B099D6F8";
	setAttr ".tan" 9;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 20 1;
	setAttr -s 2 ".kot[0:1]"  5 5;
createNode animCurveTL -n "joint2_translateX";
	rename -uid "393495AE-4940-2584-E630-018B7DC8A7C1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0.64126436912440976 20 0.64126436912440976;
createNode animCurveTL -n "joint2_translateY";
	rename -uid "ABAAAD0D-684F-0B17-A789-83B7653EB960";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 0 20 0;
createNode animCurveTL -n "joint2_translateZ";
	rename -uid "CF7C2B35-334E-4417-F2CE-81B4666ACB92";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 20 1;
createNode animCurveTU -n "joint2_scaleX";
	rename -uid "8A90A1D5-F24E-7744-8090-49998D64BA9B";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 20 1;
createNode animCurveTU -n "joint2_scaleY";
	rename -uid "6EADCDEF-5944-4372-7FF0-8D9BF0B3778F";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 20 1;
createNode animCurveTU -n "joint2_scaleZ";
	rename -uid "5FC1D1D5-6649-23E7-F2D1-8E904F7245E1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  1 1 20 1;
createNode nodeGraphEditorInfo -n "MayaNodeEditorSavedTabsInfo";
	rename -uid "06263D85-474E-4AA0-8E81-F68D40D5A9FB";
	setAttr ".tgi[0].tn" -type "string" "Untitled_1";
	setAttr ".tgi[0].vl" -type "double2" -1294.831222646887 -784.09872164545459 ;
	setAttr ".tgi[0].vh" -type "double2" 1279.2366012087432 821.5373531311966 ;
	setAttr -s 16 ".tgi[0].ni";
	setAttr ".tgi[0].ni[0].x" 48.571430206298828;
	setAttr ".tgi[0].ni[0].y" 261.42855834960938;
	setAttr ".tgi[0].ni[0].nvs" 18304;
	setAttr ".tgi[0].ni[1].x" 662.85711669921875;
	setAttr ".tgi[0].ni[1].y" -126.82987976074219;
	setAttr ".tgi[0].ni[1].nvs" 18304;
	setAttr ".tgi[0].ni[2].x" 970;
	setAttr ".tgi[0].ni[2].y" 222.85714721679688;
	setAttr ".tgi[0].ni[2].nvs" 18304;
	setAttr ".tgi[0].ni[3].x" -565.71429443359375;
	setAttr ".tgi[0].ni[3].y" 88.571426391601562;
	setAttr ".tgi[0].ni[3].nvs" 18304;
	setAttr ".tgi[0].ni[4].x" -1180;
	setAttr ".tgi[0].ni[4].y" 95.714286804199219;
	setAttr ".tgi[0].ni[4].nvs" 18304;
	setAttr ".tgi[0].ni[5].x" -258.57144165039062;
	setAttr ".tgi[0].ni[5].y" 52.857143402099609;
	setAttr ".tgi[0].ni[5].nvs" 18304;
	setAttr ".tgi[0].ni[6].x" 48.571430206298828;
	setAttr ".tgi[0].ni[6].y" 162.85714721679688;
	setAttr ".tgi[0].ni[6].nvs" 18304;
	setAttr ".tgi[0].ni[7].x" -872.85711669921875;
	setAttr ".tgi[0].ni[7].y" 95.714286804199219;
	setAttr ".tgi[0].ni[7].nvs" 18304;
	setAttr ".tgi[0].ni[8].x" -872.85711669921875;
	setAttr ".tgi[0].ni[8].y" -2.8571429252624512;
	setAttr ".tgi[0].ni[8].nvs" 18304;
	setAttr ".tgi[0].ni[9].x" -258.57144165039062;
	setAttr ".tgi[0].ni[9].y" 151.42857360839844;
	setAttr ".tgi[0].ni[9].nvs" 18304;
	setAttr ".tgi[0].ni[10].x" 355.71429443359375;
	setAttr ".tgi[0].ni[10].y" 134.28572082519531;
	setAttr ".tgi[0].ni[10].nvs" 18304;
	setAttr ".tgi[0].ni[11].x" 970;
	setAttr ".tgi[0].ni[11].y" -72.857139587402344;
	setAttr ".tgi[0].ni[11].nvs" 18304;
	setAttr ".tgi[0].ni[12].x" 970;
	setAttr ".tgi[0].ni[12].y" -171.42857360839844;
	setAttr ".tgi[0].ni[12].nvs" 18304;
	setAttr ".tgi[0].ni[13].x" 662.85711669921875;
	setAttr ".tgi[0].ni[13].y" 75.714286804199219;
	setAttr ".tgi[0].ni[13].nvs" 18304;
	setAttr ".tgi[0].ni[14].x" 970;
	setAttr ".tgi[0].ni[14].y" 25.714284896850586;
	setAttr ".tgi[0].ni[14].nvs" 18304;
	setAttr ".tgi[0].ni[15].x" 970;
	setAttr ".tgi[0].ni[15].y" 124.28571319580078;
	setAttr ".tgi[0].ni[15].nvs" 18304;
select -ne :time1;
	setAttr -av -k on ".cch";
	setAttr -av -cb on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k on ".o" 1;
	setAttr -av -k on ".unw" 1;
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
	setAttr -s 2 ".st";
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
	setAttr -s 5 ".s";
select -ne :postProcessList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 2 ".p";
select -ne :defaultRenderingList1;
	setAttr -k on ".ihi";
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
	setAttr -s 31 ".dsm";
	setAttr -cb on ".mwc";
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
	setAttr -cb on ".mwc";
	setAttr -cb on ".an";
	setAttr -cb on ".il";
	setAttr -cb on ".vo";
	setAttr -cb on ".eo";
	setAttr -cb on ".fo";
	setAttr -cb on ".epo";
	setAttr -k on ".ro" yes;
select -ne :defaultRenderGlobals;
	addAttr -ci true -h true -sn "dss" -ln "defaultSurfaceShader" -dt "string";
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k on ".macc";
	setAttr -k on ".macd";
	setAttr -k on ".macq";
	setAttr -k on ".mcfr";
	setAttr -cb on ".ifg";
	setAttr -k on ".clip";
	setAttr -k on ".edm";
	setAttr -k on ".edl";
	setAttr -cb on ".ren" -type "string" "arnold";
	setAttr -av -k on ".esr";
	setAttr -k on ".ors";
	setAttr -cb on ".sdf";
	setAttr -av -k on ".outf";
	setAttr -cb on ".imfkey";
	setAttr -k on ".gama";
	setAttr -cb on ".an";
	setAttr -cb on ".ar";
	setAttr -k on ".fs";
	setAttr -k on ".ef";
	setAttr -av -k on ".bfs";
	setAttr -cb on ".me";
	setAttr -cb on ".se";
	setAttr -k on ".be";
	setAttr -cb on ".ep";
	setAttr -k on ".fec";
	setAttr -av -k on ".ofc";
	setAttr -cb on ".ofe";
	setAttr -cb on ".efe";
	setAttr -cb on ".oft";
	setAttr -cb on ".umfn";
	setAttr -cb on ".ufe";
	setAttr -cb on ".pff";
	setAttr -cb on ".peie";
	setAttr -cb on ".ifp";
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
	setAttr -cb on ".gv";
	setAttr -cb on ".sv";
	setAttr -k on ".mm";
	setAttr -k on ".npu";
	setAttr -k on ".itf";
	setAttr -k on ".shp";
	setAttr -cb on ".isp";
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
	setAttr -cb on ".prm";
	setAttr -cb on ".pom";
	setAttr -cb on ".pfrm";
	setAttr -cb on ".pfom";
	setAttr -av -k on ".bll";
	setAttr -av -k on ".bls";
	setAttr -av -k on ".smv";
	setAttr -k on ".ubc";
	setAttr -k on ".mbc";
	setAttr -cb on ".mbt";
	setAttr -k on ".udbx";
	setAttr -k on ".smc";
	setAttr -k on ".kmv";
	setAttr -cb on ".isl";
	setAttr -cb on ".ism";
	setAttr -cb on ".imb";
	setAttr -k on ".rlen";
	setAttr -av -k on ".frts";
	setAttr -k on ".tlwd";
	setAttr -k on ".tlht";
	setAttr -k on ".jfc";
	setAttr -cb on ".rsb";
	setAttr -k on ".ope";
	setAttr -k on ".oppf";
	setAttr -k on ".rcp";
	setAttr -k on ".icp";
	setAttr -k on ".ocp";
	setAttr -cb on ".hbl";
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
	setAttr -av -k on ".dpi";
	setAttr -av -k on ".off";
	setAttr -av -k on ".fld";
	setAttr -av -k on ".zsl";
	setAttr -av -k on ".isu";
	setAttr -av -k on ".pdu";
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
connectAttr "blendShape9GroupId.id" "|basic_cube_1_blendshape_no_anim|base|baseShape.iog.og[0].gid"
		;
connectAttr "blendShape9Set.mwc" "|basic_cube_1_blendshape_no_anim|base|baseShape.iog.og[0].gco"
		;
connectAttr "groupId18.id" "|basic_cube_1_blendshape_no_anim|base|baseShape.iog.og[1].gid"
		;
connectAttr "tweakSet9.mwc" "|basic_cube_1_blendshape_no_anim|base|baseShape.iog.og[1].gco"
		;
connectAttr "blendShape9.og[0]" "|basic_cube_1_blendshape_no_anim|base|baseShape.i"
		;
connectAttr "tweak9.vl[0].vt[0]" "|basic_cube_1_blendshape_no_anim|base|baseShape.twl"
		;
connectAttr "polyCube1.out" "|basic_cube_1_blendshape_no_anim|base|baseShape1Orig.i"
		;
connectAttr "blendShape10GroupId.id" "|basic_cube_2_inbetweens_no_anim|base|baseShape.iog.og[0].gid"
		;
connectAttr "blendShape10Set.mwc" "|basic_cube_2_inbetweens_no_anim|base|baseShape.iog.og[0].gco"
		;
connectAttr "groupId20.id" "|basic_cube_2_inbetweens_no_anim|base|baseShape.iog.og[1].gid"
		;
connectAttr "tweakSet10.mwc" "|basic_cube_2_inbetweens_no_anim|base|baseShape.iog.og[1].gco"
		;
connectAttr "blendShape10.og[0]" "|basic_cube_2_inbetweens_no_anim|base|baseShape.i"
		;
connectAttr "tweak10.vl[0].vt[0]" "|basic_cube_2_inbetweens_no_anim|base|baseShape.twl"
		;
connectAttr "polyCube2.out" "|basic_cube_2_inbetweens_no_anim|base|baseShape1Orig.i"
		;
connectAttr "blendShape12GroupId.id" "|basic_cube_4_blendshapes_no_anim|base|baseShape.iog.og[0].gid"
		;
connectAttr "blendShape12Set.mwc" "|basic_cube_4_blendshapes_no_anim|base|baseShape.iog.og[0].gco"
		;
connectAttr "groupId24.id" "|basic_cube_4_blendshapes_no_anim|base|baseShape.iog.og[1].gid"
		;
connectAttr "tweakSet12.mwc" "|basic_cube_4_blendshapes_no_anim|base|baseShape.iog.og[1].gco"
		;
connectAttr "blendShape12.og[0]" "|basic_cube_4_blendshapes_no_anim|base|baseShape.i"
		;
connectAttr "tweak12.vl[0].vt[0]" "|basic_cube_4_blendshapes_no_anim|base|baseShape.twl"
		;
connectAttr "polyCube4.out" "|basic_cube_4_blendshapes_no_anim|base|baseShapeOrig.i"
		;
connectAttr "blendShape13GroupId.id" "|basic_cube_4_blendshape_inbetween_anim|base|baseShape.iog.og[2].gid"
		;
connectAttr "blendShape13Set.mwc" "|basic_cube_4_blendshape_inbetween_anim|base|baseShape.iog.og[2].gco"
		;
connectAttr "groupId26.id" "|basic_cube_4_blendshape_inbetween_anim|base|baseShape.iog.og[3].gid"
		;
connectAttr "tweakSet13.mwc" "|basic_cube_4_blendshape_inbetween_anim|base|baseShape.iog.og[3].gco"
		;
connectAttr "blendShape13.og[0]" "|basic_cube_4_blendshape_inbetween_anim|base|baseShape.i"
		;
connectAttr "tweak13.vl[0].vt[0]" "|basic_cube_4_blendshape_inbetween_anim|base|baseShape.twl"
		;
connectAttr "blendShape16GroupId.id" "|basic_cube_3_blendshape_anim|base|baseShape.iog.og[2].gid"
		;
connectAttr "blendShape16Set.mwc" "|basic_cube_3_blendshape_anim|base|baseShape.iog.og[2].gco"
		;
connectAttr "groupId32.id" "|basic_cube_3_blendshape_anim|base|baseShape.iog.og[3].gid"
		;
connectAttr "tweakSet16.mwc" "|basic_cube_3_blendshape_anim|base|baseShape.iog.og[3].gco"
		;
connectAttr "blendShape16.og[0]" "|basic_cube_3_blendshape_anim|base|baseShape.i"
		;
connectAttr "tweak16.vl[0].vt[0]" "|basic_cube_3_blendshape_anim|base|baseShape.twl"
		;
connectAttr "blendShape14GroupId.id" "|basic_cube_2_inbetween_anim|base|baseShape.iog.og[2].gid"
		;
connectAttr "blendShape14Set.mwc" "|basic_cube_2_inbetween_anim|base|baseShape.iog.og[2].gco"
		;
connectAttr "groupId28.id" "|basic_cube_2_inbetween_anim|base|baseShape.iog.og[3].gid"
		;
connectAttr "tweakSet14.mwc" "|basic_cube_2_inbetween_anim|base|baseShape.iog.og[3].gco"
		;
connectAttr "blendShape14.og[0]" "|basic_cube_2_inbetween_anim|base|baseShape.i"
		;
connectAttr "tweak14.vl[0].vt[0]" "|basic_cube_2_inbetween_anim|base|baseShape.twl"
		;
connectAttr "blendShape17GroupId.id" "pCube1baseShape.iog.og[0].gid";
connectAttr "blendShape17Set.mwc" "pCube1baseShape.iog.og[0].gco";
connectAttr "groupId34.id" "pCube1baseShape.iog.og[1].gid";
connectAttr "tweakSet17.mwc" "pCube1baseShape.iog.og[1].gco";
connectAttr "blendShape17.og[0]" "pCube1baseShape.i";
connectAttr "tweak17.vl[0].vt[0]" "pCube1baseShape.twl";
connectAttr "polyCube5.out" "pCube1baseShape1Orig.i";
connectAttr "blendShape18GroupId.id" "pCube2baseShape.iog.og[0].gid";
connectAttr "blendShape18Set.mwc" "pCube2baseShape.iog.og[0].gco";
connectAttr "groupId36.id" "pCube2baseShape.iog.og[1].gid";
connectAttr "tweakSet18.mwc" "pCube2baseShape.iog.og[1].gco";
connectAttr "blendShape18.og[0]" "pCube2baseShape.i";
connectAttr "tweak18.vl[0].vt[0]" "pCube2baseShape.twl";
connectAttr "squash1GroupId.id" "|cube_extra_deformer_in_stack|group9|base|baseShape.iog.og[3].gid"
		;
connectAttr "squash1Set.mwc" "|cube_extra_deformer_in_stack|group9|base|baseShape.iog.og[3].gco"
		;
connectAttr "groupId38.id" "|cube_extra_deformer_in_stack|group9|base|baseShape.iog.og[4].gid"
		;
connectAttr "tweakSet19.mwc" "|cube_extra_deformer_in_stack|group9|base|baseShape.iog.og[4].gco"
		;
connectAttr "blendShape19GroupId.id" "|cube_extra_deformer_in_stack|group9|base|baseShape.iog.og[5].gid"
		;
connectAttr "blendShape19Set.mwc" "|cube_extra_deformer_in_stack|group9|base|baseShape.iog.og[5].gco"
		;
connectAttr "blendShape19.og[0]" "|cube_extra_deformer_in_stack|group9|base|baseShape.i"
		;
connectAttr "tweak19.vl[0].vt[0]" "|cube_extra_deformer_in_stack|group9|base|baseShape.twl"
		;
connectAttr "polyCube6.out" "|cube_extra_deformer_in_stack|group9|base|baseShapeOrig.i"
		;
connectAttr "squash1.msg" "squash1Handle.sml";
connectAttr "blendShape12GroupId1.id" "|basic_cube_4_blendshape_baked_targets|base|baseShape.iog.og[0].gid"
		;
connectAttr "blendShape12Set1.mwc" "|basic_cube_4_blendshape_baked_targets|base|baseShape.iog.og[0].gco"
		;
connectAttr "groupId43.id" "|basic_cube_4_blendshape_baked_targets|base|baseShape.iog.og[1].gid"
		;
connectAttr "tweakSet22.mwc" "|basic_cube_4_blendshape_baked_targets|base|baseShape.iog.og[1].gco"
		;
connectAttr "blendShape22.og[0]" "|basic_cube_4_blendshape_baked_targets|base|baseShape.i"
		;
connectAttr "tweak22.vl[0].vt[0]" "|basic_cube_4_blendshape_baked_targets|base|baseShape.twl"
		;
connectAttr "polyCube7.out" "|basic_cube_4_blendshape_baked_targets|base|baseShapeOrig.i"
		;
connectAttr "skinCluster2GroupId.id" "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.iog.og[0].gid"
		;
connectAttr "skinCluster2Set.mwc" "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.iog.og[0].gco"
		;
connectAttr "groupId45.id" "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.iog.og[1].gid"
		;
connectAttr "tweakSet23.mwc" "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.iog.og[1].gco"
		;
connectAttr "blendShape23GroupId.id" "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.iog.og[2].gid"
		;
connectAttr "blendShape23Set.mwc" "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.iog.og[2].gco"
		;
connectAttr "skinCluster2.og[0]" "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.i"
		;
connectAttr "tweak23.vl[0].vt[0]" "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.twl"
		;
connectAttr "joint2_scaleX.o" "joint2.sx";
connectAttr "joint2_scaleY.o" "joint2.sy";
connectAttr "joint2_scaleZ.o" "joint2.sz";
connectAttr "joint2_rotateX.o" "joint2.rx";
connectAttr "joint2_rotateY.o" "joint2.ry";
connectAttr "joint2_rotateZ.o" "joint2.rz";
connectAttr "joint2_visibility.o" "joint2.v";
connectAttr "joint2_translateX.o" "joint2.tx";
connectAttr "joint2_translateY.o" "joint2.ty";
connectAttr "joint2_translateZ.o" "joint2.tz";
connectAttr "joint2.s" "joint3.is";
connectAttr "joint3_visibility.o" "joint3.v";
connectAttr "joint3_translateX.o" "joint3.tx";
connectAttr "joint3_translateY.o" "joint3.ty";
connectAttr "joint3_translateZ.o" "joint3.tz";
connectAttr "joint3_rotateX.o" "joint3.rx";
connectAttr "joint3_rotateY.o" "joint3.ry";
connectAttr "joint3_rotateZ.o" "joint3.rz";
connectAttr "joint3_scaleX.o" "joint3.sx";
connectAttr "joint3_scaleY.o" "joint3.sy";
connectAttr "joint3_scaleZ.o" "joint3.sz";
relationship "link" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
connectAttr "blendShape9.mlpr" "shapeEditorManager.bspr[8]";
connectAttr "blendShape10.mlpr" "shapeEditorManager.bspr[9]";
connectAttr "blendShape12.mlpr" "shapeEditorManager.bspr[11]";
connectAttr "blendShape13.mlpr" "shapeEditorManager.bspr[12]";
connectAttr "blendShape14.mlpr" "shapeEditorManager.bspr[13]";
connectAttr "blendShape16.mlpr" "shapeEditorManager.bspr[15]";
connectAttr "blendShape17.mlpr" "shapeEditorManager.bspr[16]";
connectAttr "blendShape18.mlpr" "shapeEditorManager.bspr[17]";
connectAttr "blendShape19.mlpr" "shapeEditorManager.bspr[18]";
connectAttr "blendShape22.mlpr" "shapeEditorManager.bspr[21]";
connectAttr "blendShape23.mlpr" "shapeEditorManager.bspr[22]";
connectAttr "layerManager.dli[0]" "defaultLayer.id";
connectAttr "renderLayerManager.rlmi[0]" "defaultRenderLayer.rlid";
connectAttr "blendShape9GroupParts.og" "blendShape9.ip[0].ig";
connectAttr "blendShape9GroupId.id" "blendShape9.ip[0].gi";
connectAttr "shapeEditorManager.obsv[8]" "blendShape9.tgdt[0].dpvs";
connectAttr "|basic_cube_1_blendshape_no_anim|tgt|tgtShape.w" "blendShape9.it[0].itg[0].iti[6000].igt"
		;
connectAttr "groupParts18.og" "tweak9.ip[0].ig";
connectAttr "groupId18.id" "tweak9.ip[0].gi";
connectAttr "blendShape9GroupId.msg" "blendShape9Set.gn" -na;
connectAttr "|basic_cube_1_blendshape_no_anim|base|baseShape.iog.og[0]" "blendShape9Set.dsm"
		 -na;
connectAttr "blendShape9.msg" "blendShape9Set.ub[0]";
connectAttr "tweak9.og[0]" "blendShape9GroupParts.ig";
connectAttr "blendShape9GroupId.id" "blendShape9GroupParts.gi";
connectAttr "groupId18.msg" "tweakSet9.gn" -na;
connectAttr "|basic_cube_1_blendshape_no_anim|base|baseShape.iog.og[1]" "tweakSet9.dsm"
		 -na;
connectAttr "tweak9.msg" "tweakSet9.ub[0]";
connectAttr "|basic_cube_1_blendshape_no_anim|base|baseShape1Orig.w" "groupParts18.ig"
		;
connectAttr "groupId18.id" "groupParts18.gi";
connectAttr "blendShape10GroupParts.og" "blendShape10.ip[0].ig";
connectAttr "blendShape10GroupId.id" "blendShape10.ip[0].gi";
connectAttr "shapeEditorManager.obsv[9]" "blendShape10.tgdt[0].dpvs";
connectAttr "|basic_cube_2_inbetweens_no_anim|tgts|tgt3|tgtShape3.w" "blendShape10.it[0].itg[0].iti[5333].igt"
		;
connectAttr "|basic_cube_2_inbetweens_no_anim|tgts|tgt2|tgtShape2.w" "blendShape10.it[0].itg[0].iti[5667].igt"
		;
connectAttr "|basic_cube_2_inbetweens_no_anim|tgts|tgt1|tgtShape1.w" "blendShape10.it[0].itg[0].iti[6000].igt"
		;
connectAttr "groupParts20.og" "tweak10.ip[0].ig";
connectAttr "groupId20.id" "tweak10.ip[0].gi";
connectAttr "blendShape10GroupId.msg" "blendShape10Set.gn" -na;
connectAttr "|basic_cube_2_inbetweens_no_anim|base|baseShape.iog.og[0]" "blendShape10Set.dsm"
		 -na;
connectAttr "blendShape10.msg" "blendShape10Set.ub[0]";
connectAttr "tweak10.og[0]" "blendShape10GroupParts.ig";
connectAttr "blendShape10GroupId.id" "blendShape10GroupParts.gi";
connectAttr "groupId20.msg" "tweakSet10.gn" -na;
connectAttr "|basic_cube_2_inbetweens_no_anim|base|baseShape.iog.og[1]" "tweakSet10.dsm"
		 -na;
connectAttr "tweak10.msg" "tweakSet10.ub[0]";
connectAttr "|basic_cube_2_inbetweens_no_anim|base|baseShape1Orig.w" "groupParts20.ig"
		;
connectAttr "groupId20.id" "groupParts20.gi";
connectAttr "blendShape13_tgt.o" "blendShape13.w[0]";
connectAttr "blendShape13GroupParts.og" "blendShape13.ip[0].ig";
connectAttr "blendShape13GroupId.id" "blendShape13.ip[0].gi";
connectAttr "shapeEditorManager.obsv[12]" "blendShape13.tgdt[0].dpvs";
connectAttr "|basic_cube_4_blendshape_inbetween_anim|tgts|tgt3|tgt3Shape.w" "blendShape13.it[0].itg[0].iti[5333].igt"
		;
connectAttr "|basic_cube_4_blendshape_inbetween_anim|tgts|tgt2|tgt2Shape.w" "blendShape13.it[0].itg[0].iti[5667].igt"
		;
connectAttr "|basic_cube_4_blendshape_inbetween_anim|tgts|tgt|tgtShape.w" "blendShape13.it[0].itg[0].iti[6000].igt"
		;
connectAttr "groupParts26.og" "tweak13.ip[0].ig";
connectAttr "groupId26.id" "tweak13.ip[0].gi";
connectAttr "blendShape13GroupId.msg" "blendShape13Set.gn" -na;
connectAttr "|basic_cube_4_blendshape_inbetween_anim|base|baseShape.iog.og[2]" "blendShape13Set.dsm"
		 -na;
connectAttr "blendShape13.msg" "blendShape13Set.ub[0]";
connectAttr "tweak13.og[0]" "blendShape13GroupParts.ig";
connectAttr "blendShape13GroupId.id" "blendShape13GroupParts.gi";
connectAttr "groupId26.msg" "tweakSet13.gn" -na;
connectAttr "|basic_cube_4_blendshape_inbetween_anim|base|baseShape.iog.og[3]" "tweakSet13.dsm"
		 -na;
connectAttr "tweak13.msg" "tweakSet13.ub[0]";
connectAttr "baseShapeOrig1.w" "groupParts26.ig";
connectAttr "groupId26.id" "groupParts26.gi";
connectAttr "blendShape14GroupParts.og" "blendShape14.ip[0].ig";
connectAttr "blendShape14GroupId.id" "blendShape14.ip[0].gi";
connectAttr "shapeEditorManager.obsv[13]" "blendShape14.tgdt[0].dpvs";
connectAttr "|basic_cube_2_inbetween_anim|tgts|tgt3|tgtShape3.w" "blendShape14.it[0].itg[0].iti[6000].igt"
		;
connectAttr "|basic_cube_2_inbetween_anim|tgts|tgt2|tgtShape2.w" "blendShape14.it[0].itg[1].iti[6000].igt"
		;
connectAttr "|basic_cube_2_inbetween_anim|tgts|tgt1|tgtShape1.w" "blendShape14.it[0].itg[2].iti[6000].igt"
		;
connectAttr "blendShape14_tgt3.o" "blendShape14.w[0]";
connectAttr "blendShape14_tgt2.o" "blendShape14.w[1]";
connectAttr "blendShape14_tgt1.o" "blendShape14.w[2]";
connectAttr "groupParts28.og" "tweak14.ip[0].ig";
connectAttr "groupId28.id" "tweak14.ip[0].gi";
connectAttr "blendShape14GroupId.msg" "blendShape14Set.gn" -na;
connectAttr "|basic_cube_2_inbetween_anim|base|baseShape.iog.og[2]" "blendShape14Set.dsm"
		 -na;
connectAttr "blendShape14.msg" "blendShape14Set.ub[0]";
connectAttr "tweak14.og[0]" "blendShape14GroupParts.ig";
connectAttr "blendShape14GroupId.id" "blendShape14GroupParts.gi";
connectAttr "groupId28.msg" "tweakSet14.gn" -na;
connectAttr "|basic_cube_2_inbetween_anim|base|baseShape.iog.og[3]" "tweakSet14.dsm"
		 -na;
connectAttr "tweak14.msg" "tweakSet14.ub[0]";
connectAttr "|basic_cube_2_inbetween_anim|base|baseShapeOrig.w" "groupParts28.ig"
		;
connectAttr "groupId28.id" "groupParts28.gi";
connectAttr "blendShape16GroupParts.og" "blendShape16.ip[0].ig";
connectAttr "blendShape16GroupId.id" "blendShape16.ip[0].gi";
connectAttr "shapeEditorManager.obsv[15]" "blendShape16.tgdt[0].dpvs";
connectAttr "blendShape16_targetDirectory_0__directoryWeight.o" "blendShape16.tgdt[0].dwgh"
		;
connectAttr "|basic_cube_3_blendshape_anim|tgt|tgtShape.w" "blendShape16.it[0].itg[0].iti[6000].igt"
		;
connectAttr "blendShape16_envelope.o" "blendShape16.en";
connectAttr "blendShape16_tgt.o" "blendShape16.w[0]";
connectAttr "groupParts32.og" "tweak16.ip[0].ig";
connectAttr "groupId32.id" "tweak16.ip[0].gi";
connectAttr "blendShape16GroupId.msg" "blendShape16Set.gn" -na;
connectAttr "|basic_cube_3_blendshape_anim|base|baseShape.iog.og[2]" "blendShape16Set.dsm"
		 -na;
connectAttr "blendShape16.msg" "blendShape16Set.ub[0]";
connectAttr "tweak16.og[0]" "blendShape16GroupParts.ig";
connectAttr "blendShape16GroupId.id" "blendShape16GroupParts.gi";
connectAttr "groupId32.msg" "tweakSet16.gn" -na;
connectAttr "|basic_cube_3_blendshape_anim|base|baseShape.iog.og[3]" "tweakSet16.dsm"
		 -na;
connectAttr "tweak16.msg" "tweakSet16.ub[0]";
connectAttr "baseShapeOrig2.w" "groupParts32.ig";
connectAttr "groupId32.id" "groupParts32.gi";
connectAttr "blendShape17GroupParts.og" "blendShape17.ip[0].ig";
connectAttr "blendShape17GroupId.id" "blendShape17.ip[0].gi";
connectAttr "shapeEditorManager.obsv[16]" "blendShape17.tgdt[0].dpvs";
connectAttr "pCube1tgtShape.w" "blendShape17.it[0].itg[0].iti[6000].igt";
connectAttr "groupParts34.og" "tweak17.ip[0].ig";
connectAttr "groupId34.id" "tweak17.ip[0].gi";
connectAttr "blendShape17GroupId.msg" "blendShape17Set.gn" -na;
connectAttr "pCube1baseShape.iog.og[0]" "blendShape17Set.dsm" -na;
connectAttr "blendShape17.msg" "blendShape17Set.ub[0]";
connectAttr "tweak17.og[0]" "blendShape17GroupParts.ig";
connectAttr "blendShape17GroupId.id" "blendShape17GroupParts.gi";
connectAttr "groupId34.msg" "tweakSet17.gn" -na;
connectAttr "pCube1baseShape.iog.og[1]" "tweakSet17.dsm" -na;
connectAttr "tweak17.msg" "tweakSet17.ub[0]";
connectAttr "pCube1baseShape1Orig.w" "groupParts34.ig";
connectAttr "groupId34.id" "groupParts34.gi";
connectAttr "blendShape18GroupParts.og" "blendShape18.ip[0].ig";
connectAttr "blendShape18GroupId.id" "blendShape18.ip[0].gi";
connectAttr "shapeEditorManager.obsv[17]" "blendShape18.tgdt[0].dpvs";
connectAttr "pCube2tgtShape.w" "blendShape18.it[0].itg[0].iti[6000].igt";
connectAttr "blendShape17.w[0]" "blendShape18.w[0]";
connectAttr "groupParts36.og" "tweak18.ip[0].ig";
connectAttr "groupId36.id" "tweak18.ip[0].gi";
connectAttr "blendShape18GroupId.msg" "blendShape18Set.gn" -na;
connectAttr "pCube2baseShape.iog.og[0]" "blendShape18Set.dsm" -na;
connectAttr "blendShape18.msg" "blendShape18Set.ub[0]";
connectAttr "tweak18.og[0]" "blendShape18GroupParts.ig";
connectAttr "blendShape18GroupId.id" "blendShape18GroupParts.gi";
connectAttr "groupId36.msg" "tweakSet18.gn" -na;
connectAttr "pCube2baseShape.iog.og[1]" "tweakSet18.dsm" -na;
connectAttr "tweak18.msg" "tweakSet18.ub[0]";
connectAttr "pCube2baseShape2Orig.w" "groupParts36.ig";
connectAttr "groupId36.id" "groupParts36.gi";
connectAttr "squash1HandleShape.fac" "squash1.fac";
connectAttr "squash1HandleShape.exp" "squash1.exp";
connectAttr "squash1HandleShape.mp" "squash1.mp";
connectAttr "squash1HandleShape.ss" "squash1.ss";
connectAttr "squash1HandleShape.es" "squash1.es";
connectAttr "squash1HandleShape.lb" "squash1.lb";
connectAttr "squash1HandleShape.hb" "squash1.hb";
connectAttr "squash1GroupParts.og" "squash1.ip[0].ig";
connectAttr "squash1GroupId.id" "squash1.ip[0].gi";
connectAttr "squash1HandleShape.dd" "squash1.dd";
connectAttr "squash1Handle.wm" "squash1.ma";
connectAttr "groupParts38.og" "tweak19.ip[0].ig";
connectAttr "groupId38.id" "tweak19.ip[0].gi";
connectAttr "squash1GroupId.msg" "squash1Set.gn" -na;
connectAttr "|cube_extra_deformer_in_stack|group9|base|baseShape.iog.og[3]" "squash1Set.dsm"
		 -na;
connectAttr "squash1.msg" "squash1Set.ub[0]";
connectAttr "tweak19.og[0]" "squash1GroupParts.ig";
connectAttr "squash1GroupId.id" "squash1GroupParts.gi";
connectAttr "groupId38.msg" "tweakSet19.gn" -na;
connectAttr "|cube_extra_deformer_in_stack|group9|base|baseShape.iog.og[4]" "tweakSet19.dsm"
		 -na;
connectAttr "tweak19.msg" "tweakSet19.ub[0]";
connectAttr "|cube_extra_deformer_in_stack|group9|base|baseShapeOrig.w" "groupParts38.ig"
		;
connectAttr "groupId38.id" "groupParts38.gi";
connectAttr "blendShape19GroupParts.og" "blendShape19.ip[0].ig";
connectAttr "blendShape19GroupId.id" "blendShape19.ip[0].gi";
connectAttr "shapeEditorManager.obsv[18]" "blendShape19.tgdt[0].dpvs";
connectAttr "|cube_extra_deformer_in_stack|group9|tgt|tgtShape.w" "blendShape19.it[0].itg[0].iti[6000].igt"
		;
connectAttr "blendShape19GroupId.msg" "blendShape19Set.gn" -na;
connectAttr "|cube_extra_deformer_in_stack|group9|base|baseShape.iog.og[5]" "blendShape19Set.dsm"
		 -na;
connectAttr "blendShape19.msg" "blendShape19Set.ub[0]";
connectAttr "squash1.og[0]" "blendShape19GroupParts.ig";
connectAttr "blendShape19GroupId.id" "blendShape19GroupParts.gi";
connectAttr "blendShape12GroupParts.og" "blendShape12.ip[0].ig";
connectAttr "blendShape12GroupId.id" "blendShape12.ip[0].gi";
connectAttr "shapeEditorManager.obsv[11]" "blendShape12.tgdt[0].dpvs";
connectAttr "|basic_cube_4_blendshapes_no_anim|tgts|tgt3|tgt3Shape.w" "blendShape12.it[0].itg[0].iti[6000].igt"
		;
connectAttr "|basic_cube_4_blendshapes_no_anim|tgts|tgt2|tgt2Shape.w" "blendShape12.it[0].itg[1].iti[6000].igt"
		;
connectAttr "|basic_cube_4_blendshapes_no_anim|tgts|tgt|tgtShape.w" "blendShape12.it[0].itg[2].iti[6000].igt"
		;
connectAttr "blendShape12GroupId.msg" "blendShape12Set.gn" -na;
connectAttr "|basic_cube_4_blendshapes_no_anim|base|baseShape.iog.og[0]" "blendShape12Set.dsm"
		 -na;
connectAttr "blendShape12.msg" "blendShape12Set.ub[0]";
connectAttr "tweak12.og[0]" "blendShape12GroupParts.ig";
connectAttr "blendShape12GroupId.id" "blendShape12GroupParts.gi";
connectAttr "groupParts24.og" "tweak12.ip[0].ig";
connectAttr "groupId24.id" "tweak12.ip[0].gi";
connectAttr "groupId24.msg" "tweakSet12.gn" -na;
connectAttr "|basic_cube_4_blendshapes_no_anim|base|baseShape.iog.og[1]" "tweakSet12.dsm"
		 -na;
connectAttr "tweak12.msg" "tweakSet12.ub[0]";
connectAttr "|basic_cube_4_blendshapes_no_anim|base|baseShapeOrig.w" "groupParts24.ig"
		;
connectAttr "groupId24.id" "groupParts24.gi";
connectAttr "blendShape12GroupId1.msg" "blendShape12Set1.gn" -na;
connectAttr "|basic_cube_4_blendshape_baked_targets|base|baseShape.iog.og[0]" "blendShape12Set1.dsm"
		 -na;
connectAttr "blendShape22.msg" "blendShape12Set1.ub[0]";
connectAttr "blendShape12GroupParts1.og" "blendShape22.ip[0].ig";
connectAttr "blendShape12GroupId1.id" "blendShape22.ip[0].gi";
connectAttr "shapeEditorManager.obsv[21]" "blendShape22.tgdt[0].dpvs";
connectAttr "tweak22.og[0]" "blendShape12GroupParts1.ig";
connectAttr "blendShape12GroupId1.id" "blendShape12GroupParts1.gi";
connectAttr "groupParts43.og" "tweak22.ip[0].ig";
connectAttr "groupId43.id" "tweak22.ip[0].gi";
connectAttr "groupId43.msg" "tweakSet22.gn" -na;
connectAttr "|basic_cube_4_blendshape_baked_targets|base|baseShape.iog.og[1]" "tweakSet22.dsm"
		 -na;
connectAttr "tweak22.msg" "tweakSet22.ub[0]";
connectAttr "|basic_cube_4_blendshape_baked_targets|base|baseShapeOrig.w" "groupParts43.ig"
		;
connectAttr "groupId43.id" "groupParts43.gi";
connectAttr "blendShape23.og[0]" "skinCluster2.ip[0].ig";
connectAttr "skinCluster2GroupId.id" "skinCluster2.ip[0].gi";
connectAttr "bindPose2.msg" "skinCluster2.bp";
connectAttr "joint2.wm" "skinCluster2.ma[0]";
connectAttr "joint3.wm" "skinCluster2.ma[1]";
connectAttr "joint2.liw" "skinCluster2.lw[0]";
connectAttr "joint3.liw" "skinCluster2.lw[1]";
connectAttr "joint2.obcc" "skinCluster2.ifcl[0]";
connectAttr "joint3.obcc" "skinCluster2.ifcl[1]";
connectAttr "groupParts45.og" "tweak23.ip[0].ig";
connectAttr "groupId45.id" "tweak23.ip[0].gi";
connectAttr "skinCluster2GroupId.msg" "skinCluster2Set.gn" -na;
connectAttr "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.iog.og[0]" "skinCluster2Set.dsm"
		 -na;
connectAttr "skinCluster2.msg" "skinCluster2Set.ub[0]";
connectAttr "tweak23.og[0]" "skinCluster2GroupParts.ig";
connectAttr "skinCluster2GroupId.id" "skinCluster2GroupParts.gi";
connectAttr "groupId45.msg" "tweakSet23.gn" -na;
connectAttr "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.iog.og[1]" "tweakSet23.dsm"
		 -na;
connectAttr "tweak23.msg" "tweakSet23.ub[0]";
connectAttr "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShapeOrig.w" "groupParts45.ig"
		;
connectAttr "groupId45.id" "groupParts45.gi";
connectAttr "basic_skinned_cube_blendshape_baked_target_anim.msg" "bindPose2.m[0]"
		;
connectAttr "joint2.msg" "bindPose2.m[1]";
connectAttr "joint3.msg" "bindPose2.m[2]";
connectAttr "bindPose2.w" "bindPose2.p[0]";
connectAttr "bindPose2.m[0]" "bindPose2.p[1]";
connectAttr "bindPose2.m[1]" "bindPose2.p[2]";
connectAttr "joint2.bps" "bindPose2.wm[1]";
connectAttr "joint3.bps" "bindPose2.wm[2]";
connectAttr "blendShape23GroupParts.og" "blendShape23.ip[0].ig";
connectAttr "blendShape23GroupId.id" "blendShape23.ip[0].gi";
connectAttr "shapeEditorManager.obsv[22]" "blendShape23.tgdt[0].dpvs";
connectAttr "blendShape23_tgt.o" "blendShape23.w[0]";
connectAttr "blendShape23GroupId.msg" "blendShape23Set.gn" -na;
connectAttr "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.iog.og[2]" "blendShape23Set.dsm"
		 -na;
connectAttr "blendShape23.msg" "blendShape23Set.ub[0]";
connectAttr "skinCluster2GroupParts.og" "blendShape23GroupParts.ig";
connectAttr "blendShape23GroupId.id" "blendShape23GroupParts.gi";
connectAttr "blendShape17GroupParts.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[0].dn"
		;
connectAttr "pCube1baseShape.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[1].dn";
connectAttr "blendShape17Set.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[2].dn";
connectAttr "groupParts34.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[3].dn";
connectAttr "polyCube5.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[4].dn";
connectAttr "tweak17.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[5].dn";
connectAttr "pCube1tgtShape.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[6].dn";
connectAttr "pCube1baseShape1Orig.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[7].dn"
		;
connectAttr "groupId34.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[8].dn";
connectAttr "blendShape17GroupId.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[9].dn"
		;
connectAttr "blendShape17.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[10].dn";
connectAttr "tweakSet17.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[11].dn";
connectAttr ":initialShadingGroup.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[12].dn"
		;
connectAttr "blendShape18.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[13].dn";
connectAttr "pCube2baseShape.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[14].dn"
		;
connectAttr "shapeEditorManager.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[15].dn"
		;
connectAttr "defaultRenderLayer.msg" ":defaultRenderingList1.r" -na;
connectAttr "|basic_cube_1_blendshape_no_anim|base|baseShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_1_blendshape_no_anim|tgt|tgtShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_2_inbetweens_no_anim|base|baseShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_2_inbetweens_no_anim|tgts|tgt1|tgtShape1.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_2_inbetweens_no_anim|tgts|tgt2|tgtShape2.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_2_inbetweens_no_anim|tgts|tgt3|tgtShape3.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshapes_no_anim|base|baseShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshapes_no_anim|tgts|tgt|tgtShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshapes_no_anim|tgts|tgt2|tgt2Shape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshapes_no_anim|tgts|tgt3|tgt3Shape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshape_inbetween_anim|base|baseShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshape_inbetween_anim|tgts|tgt|tgtShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshape_inbetween_anim|tgts|tgt2|tgt2Shape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshape_inbetween_anim|tgts|tgt3|tgt3Shape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_3_blendshape_anim|base|baseShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_3_blendshape_anim|tgt|tgtShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_2_inbetween_anim|base|baseShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_2_inbetween_anim|tgts|tgt1|tgtShape1.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_2_inbetween_anim|tgts|tgt2|tgtShape2.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_2_inbetween_anim|tgts|tgt3|tgtShape3.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "pCube1baseShape.iog" ":initialShadingGroup.dsm" -na;
connectAttr "pCube2baseShape.iog" ":initialShadingGroup.dsm" -na;
connectAttr "pCube1tgtShape.iog" ":initialShadingGroup.dsm" -na;
connectAttr "pCube2tgtShape.iog" ":initialShadingGroup.dsm" -na;
connectAttr "|cube_extra_deformer_in_stack|group9|base|baseShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|cube_extra_deformer_in_stack|group9|tgt|tgtShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshape_baked_targets|base|baseShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshape_baked_targets|tgts|tgt|tgtShape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshape_baked_targets|tgts|tgt2|tgt2Shape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_cube_4_blendshape_baked_targets|tgts|tgt3|tgt3Shape.iog" ":initialShadingGroup.dsm"
		 -na;
connectAttr "|basic_skinned_cube_blendshape_baked_target_anim|base|baseShape.iog" ":initialShadingGroup.dsm"
		 -na;
// End of blendShapesExport.ma
