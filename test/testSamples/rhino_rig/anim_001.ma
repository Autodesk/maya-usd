//Maya ASCII 2022 scene
//Name: anim_001.ma
//Last modified: Mon, Feb 22, 2021 01:09:37 PM
//Codeset: 1252
file -rdi 1 -ns "rhino_rig_030" -rfn "rhino_rig_029RN" -op "v=0;" -typ "mayaAscii"
		 "/scenes/rhino_model_textured/waking_animation/rhino_rig_001.ma";
file -r -ns "rhino_rig_030" -dr 1 -rfn "rhino_rig_029RN" -op "v=0;" -typ "mayaAscii"
		 "/scenes/rhino_model_textured/waking_animation/rhino_rig_001.ma";
requires maya "2022";
requires "stereoCamera" "10.0";
requires "Mayatomr" "2013.0 - 3.10.1.4 ";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2022";
fileInfo "version" "Preview Release 123";
fileInfo "cutIdentifier" "202102110315-c3c985ef4e";
fileInfo "osv" "Windows 10 Enterprise v1909 (Build: 18363)";
fileInfo "UUID" "4B0A1A39-419B-F0D3-783B-11AA728435D8";
createNode transform -s -n "persp";
	rename -uid "7CF15D9C-4A57-C267-384B-2C9A27840FEF";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 502.64670135842016 99.277741265728224 154.03692250288901 ;
	setAttr ".r" -type "double3" -3.3383527298304809 61.800000000000509 -4.2066309562858749e-16 ;
createNode camera -s -n "perspShape" -p "persp";
	rename -uid "1EC0F8D8-4927-A435-C1DC-8EAA44826E29";
	setAttr -k off ".v" no;
	setAttr ".fl" 34.999999999999993;
	setAttr ".coi" 547.90612314442069;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".tp" -type "double3" 0.22544188558163403 0.79713943361417405 -0.020910921386658676 ;
	setAttr ".hc" -type "string" "viewSet -p %camera";
createNode transform -s -n "top";
	rename -uid "2890329A-43B7-C86C-9CC7-15BF3685F1AE";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 1000.1 0 ;
	setAttr ".r" -type "double3" -90 0 0 ;
createNode camera -s -n "topShape" -p "top";
	rename -uid "5C4E9FCB-490F-D806-2AA0-33A048293CB8";
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
	rename -uid "D06DF2AA-4A9B-8387-F7B5-459C672C78F1";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 0 1000.1 ;
createNode camera -s -n "frontShape" -p "front";
	rename -uid "73C24852-490E-9E49-7E5A-E7A0AE2FFFB0";
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
	rename -uid "F5D9076A-441B-EA42-383D-25875A3A7985";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 1000.1 0 0 ;
	setAttr ".r" -type "double3" 0 90 0 ;
createNode camera -s -n "sideShape" -p "side";
	rename -uid "EF181764-475C-745B-95A1-18B140787BCA";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "side";
	setAttr ".den" -type "string" "side_depth";
	setAttr ".man" -type "string" "side_mask";
	setAttr ".hc" -type "string" "viewSet -s %camera";
	setAttr ".o" yes;
createNode lightLinker -s -n "lightLinker1";
	rename -uid "CC40356F-402D-82B2-0C3D-5188DB4C6164";
	setAttr -s 8 ".lnk";
	setAttr -s 8 ".slnk";
createNode shapeEditorManager -n "shapeEditorManager";
	rename -uid "F8C0C178-4B6A-2AF7-B881-AF831216879E";
	setAttr ".bsdt[0].bscd" -type "Int32Array" 1 0 ;
createNode poseInterpolatorManager -n "poseInterpolatorManager";
	rename -uid "8026D827-4031-9F11-9B34-C588BC926CC2";
createNode displayLayerManager -n "layerManager";
	rename -uid "33B557BD-4190-DAC5-222E-CF98C352EA89";
	setAttr ".cdl" 3;
	setAttr -s 14 ".dli[1:13]"  1 2 3 4 5 6 7 8 
		9 10 11 12 13;
createNode displayLayer -n "defaultLayer";
	rename -uid "960FD2B8-4A26-3A51-5F80-878F321177EE";
createNode renderLayerManager -n "renderLayerManager";
	rename -uid "933E5CE5-40CB-9F8D-7A51-C093ED83280C";
createNode renderLayer -n "defaultRenderLayer";
	rename -uid "0E41E2BF-4FB8-9C19-6035-A9A0F1271657";
	setAttr ".g" yes;
createNode reference -n "rhino_rig_029RN";
	rename -uid "D141B6DA-4EE2-AB32-CE02-DF8DF49A309F";
	setAttr -s 4 ".fn";
	setAttr ".fn[0]" -type "string" "C:/Users/zhangjack/3D Objects/maya_project/Zoo_tools_maya_scenes_001//scenes/rhino_model_textured/waking_animation/rhino_rig_001.ma";
	setAttr ".fn[1]" -type "string" "C:/Users/zhangjack/3D Objects/maya_project/Zoo_tools_maya_scenes/scenes/rhino_model_textured/rig/rhino_rig_031.ma";
	setAttr ".fn[2]" -type "string" "C:/Users/zhangjack/3D Objects/maya_project/Zoo_tools_maya_scenes/scenes/rhino_model_textured/rig/rhino_rig_030.ma";
	setAttr ".fn[3]" -type "string" "C:/Users/zhangjack/3D Objects/maya_project/Zoo_tools_maya_scenes/scenes/rhino_model_textured/rig/rhino_rig_029.ma";
	setAttr -s 116 ".phl";
	setAttr ".phl[1]" 0;
	setAttr ".phl[2]" 0;
	setAttr ".phl[3]" 0;
	setAttr ".phl[4]" 0;
	setAttr ".phl[5]" 0;
	setAttr ".phl[6]" 0;
	setAttr ".phl[7]" 0;
	setAttr ".phl[8]" 0;
	setAttr ".phl[9]" 0;
	setAttr ".phl[10]" 0;
	setAttr ".phl[11]" 0;
	setAttr ".phl[12]" 0;
	setAttr ".phl[13]" 0;
	setAttr ".phl[14]" 0;
	setAttr ".phl[15]" 0;
	setAttr ".phl[16]" 0;
	setAttr ".phl[17]" 0;
	setAttr ".phl[18]" 0;
	setAttr ".phl[19]" 0;
	setAttr ".phl[20]" 0;
	setAttr ".phl[21]" 0;
	setAttr ".phl[22]" 0;
	setAttr ".phl[23]" 0;
	setAttr ".phl[24]" 0;
	setAttr ".phl[25]" 0;
	setAttr ".phl[26]" 0;
	setAttr ".phl[27]" 0;
	setAttr ".phl[28]" 0;
	setAttr ".phl[29]" 0;
	setAttr ".phl[30]" 0;
	setAttr ".phl[31]" 0;
	setAttr ".phl[32]" 0;
	setAttr ".phl[33]" 0;
	setAttr ".phl[34]" 0;
	setAttr ".phl[35]" 0;
	setAttr ".phl[36]" 0;
	setAttr ".phl[37]" 0;
	setAttr ".phl[38]" 0;
	setAttr ".phl[39]" 0;
	setAttr ".phl[40]" 0;
	setAttr ".phl[41]" 0;
	setAttr ".phl[42]" 0;
	setAttr ".phl[43]" 0;
	setAttr ".phl[44]" 0;
	setAttr ".phl[45]" 0;
	setAttr ".phl[46]" 0;
	setAttr ".phl[47]" 0;
	setAttr ".phl[48]" 0;
	setAttr ".phl[49]" 0;
	setAttr ".phl[50]" 0;
	setAttr ".phl[51]" 0;
	setAttr ".phl[52]" 0;
	setAttr ".phl[53]" 0;
	setAttr ".phl[54]" 0;
	setAttr ".phl[55]" 0;
	setAttr ".phl[56]" 0;
	setAttr ".phl[57]" 0;
	setAttr ".phl[58]" 0;
	setAttr ".phl[59]" 0;
	setAttr ".phl[60]" 0;
	setAttr ".phl[61]" 0;
	setAttr ".phl[62]" 0;
	setAttr ".phl[63]" 0;
	setAttr ".phl[64]" 0;
	setAttr ".phl[65]" 0;
	setAttr ".phl[66]" 0;
	setAttr ".phl[67]" 0;
	setAttr ".phl[68]" 0;
	setAttr ".phl[69]" 0;
	setAttr ".phl[70]" 0;
	setAttr ".phl[71]" 0;
	setAttr ".phl[72]" 0;
	setAttr ".phl[73]" 0;
	setAttr ".phl[74]" 0;
	setAttr ".phl[75]" 0;
	setAttr ".phl[76]" 0;
	setAttr ".phl[77]" 0;
	setAttr ".phl[78]" 0;
	setAttr ".phl[79]" 0;
	setAttr ".phl[80]" 0;
	setAttr ".phl[81]" 0;
	setAttr ".phl[82]" 0;
	setAttr ".phl[83]" 0;
	setAttr ".phl[84]" 0;
	setAttr ".phl[85]" 0;
	setAttr ".phl[86]" 0;
	setAttr ".phl[87]" 0;
	setAttr ".phl[88]" 0;
	setAttr ".phl[89]" 0;
	setAttr ".phl[90]" 0;
	setAttr ".phl[91]" 0;
	setAttr ".phl[92]" 0;
	setAttr ".phl[93]" 0;
	setAttr ".phl[94]" 0;
	setAttr ".phl[95]" 0;
	setAttr ".phl[96]" 0;
	setAttr ".phl[97]" 0;
	setAttr ".phl[98]" 0;
	setAttr ".phl[99]" 0;
	setAttr ".phl[100]" 0;
	setAttr ".phl[101]" 0;
	setAttr ".phl[102]" 0;
	setAttr ".phl[103]" 0;
	setAttr ".phl[104]" 0;
	setAttr ".phl[105]" 0;
	setAttr ".phl[106]" 0;
	setAttr ".phl[107]" 0;
	setAttr ".phl[108]" 0;
	setAttr ".phl[109]" 0;
	setAttr ".phl[110]" 0;
	setAttr ".phl[111]" 0;
	setAttr ".phl[112]" 0;
	setAttr ".phl[113]" 0;
	setAttr ".phl[114]" 0;
	setAttr ".phl[115]" 0;
	setAttr ".phl[116]" 0;
	setAttr ".ed" -type "dataReferenceEdits" 
		"rhino_rig_029RN"
		"rhino_rig_029RN" 0
		"rhino_rig_029RN" 182
		2 "|rhino_rig_030:camera1" "visibility" " 1"
		2 "|rhino_rig_030:environment" "visibility" " 1"
		2 "|rhino_rig_030:model_grp" "visibility" " 1"
		2 "|rhino_rig_030:rig_grp" "visibility" " 1"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_neck_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_neck_jnt_001_cnt_grp|rhino_rig_030:cn_neck_jnt_001_cnt_Shift|rhino_rig_030:cn_neck_jnt_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_spine_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_spine_cnt_001_grp|rhino_rig_030:cn_spine_cnt_001_Shift|rhino_rig_030:cn_spine_cnt_001_Offset|rhino_rig_030:cn_spine_cnt_001" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_spine_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_spine_cnt_001_grp|rhino_rig_030:cn_spine_cnt_001_Shift|rhino_rig_030:cn_spine_cnt_001_Offset|rhino_rig_030:cn_spine_cnt_001" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:body_main_grp|rhino_rig_030:control_grp|rhino_rig_030:body_main_cnt_grp|rhino_rig_030:body_main_cnt_Shift|rhino_rig_030:body_main_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:body_main_grp|rhino_rig_030:control_grp|rhino_rig_030:body_main_cnt_grp|rhino_rig_030:body_main_cnt_Shift|rhino_rig_030:body_main_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:body_main_grp|rhino_rig_030:control_grp|rhino_rig_030:body_main_cnt_grp|rhino_rig_030:body_main_cnt_Shift|rhino_rig_030:body_main_cnt|rhino_rig_030:body_main_front_cnt_grp|rhino_rig_030:body_main_front_cnt_Shift|rhino_rig_030:body_main_front_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:body_main_grp|rhino_rig_030:control_grp|rhino_rig_030:body_main_cnt_grp|rhino_rig_030:body_main_cnt_Shift|rhino_rig_030:body_main_cnt|rhino_rig_030:body_main_front_cnt_grp|rhino_rig_030:body_main_front_cnt_Shift|rhino_rig_030:body_main_front_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:body_main_grp|rhino_rig_030:control_grp|rhino_rig_030:body_main_cnt_grp|rhino_rig_030:body_main_cnt_Shift|rhino_rig_030:body_main_cnt|rhino_rig_030:body_main_front_cnt_grp|rhino_rig_030:body_main_front_cnt_Shift|rhino_rig_030:body_main_front_cnt|rhino_rig_030:body_main_back_cnt_grp|rhino_rig_030:body_main_back_cnt_Shift|rhino_rig_030:body_main_back_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:body_main_grp|rhino_rig_030:control_grp|rhino_rig_030:body_main_cnt_grp|rhino_rig_030:body_main_cnt_Shift|rhino_rig_030:body_main_cnt|rhino_rig_030:body_main_front_cnt_grp|rhino_rig_030:body_main_front_cnt_Shift|rhino_rig_030:body_main_front_cnt|rhino_rig_030:body_main_back_cnt_grp|rhino_rig_030:body_main_back_cnt_Shift|rhino_rig_030:body_main_back_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_head_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_head_jnt_001_cnt_grp|rhino_rig_030:cn_head_jnt_001_cnt_Shift|rhino_rig_030:cn_head_jnt_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_head_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_head_jnt_001_cnt_grp|rhino_rig_030:cn_head_jnt_001_cnt_Shift|rhino_rig_030:cn_head_jnt_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_main_001_cnt_grp|rhino_rig_030:lf_ft_scapular_main_001_cnt_Shift|rhino_rig_030:lf_ft_scapular_main_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_main_001_cnt_grp|rhino_rig_030:lf_ft_scapular_main_001_cnt_Shift|rhino_rig_030:lf_ft_scapular_main_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_leg_vector_001_cnt_grp|rhino_rig_030:lf_ft_leg_vector_001_cnt_Shift|rhino_rig_030:lf_ft_leg_vector_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt|rhino_rig_030:lf_ft_left_roll_001_cnt_grp|rhino_rig_030:lf_ft_left_roll_001_cnt_Shift|rhino_rig_030:lf_ft_left_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt|rhino_rig_030:lf_ft_left_roll_001_cnt_grp|rhino_rig_030:lf_ft_left_roll_001_cnt_Shift|rhino_rig_030:lf_ft_left_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt|rhino_rig_030:lf_ft_left_roll_001_cnt_grp|rhino_rig_030:lf_ft_left_roll_001_cnt_Shift|rhino_rig_030:lf_ft_left_roll_001_cnt|rhino_rig_030:lf_ft_right_roll_001_cnt_grp|rhino_rig_030:lf_ft_right_roll_001_cnt_Shift|rhino_rig_030:lf_ft_right_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt|rhino_rig_030:lf_ft_left_roll_001_cnt_grp|rhino_rig_030:lf_ft_left_roll_001_cnt_Shift|rhino_rig_030:lf_ft_left_roll_001_cnt|rhino_rig_030:lf_ft_right_roll_001_cnt_grp|rhino_rig_030:lf_ft_right_roll_001_cnt_Shift|rhino_rig_030:lf_ft_right_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt|rhino_rig_030:lf_ft_left_roll_001_cnt_grp|rhino_rig_030:lf_ft_left_roll_001_cnt_Shift|rhino_rig_030:lf_ft_left_roll_001_cnt|rhino_rig_030:lf_ft_right_roll_001_cnt_grp|rhino_rig_030:lf_ft_right_roll_001_cnt_Shift|rhino_rig_030:lf_ft_right_roll_001_cnt|rhino_rig_030:lf_ft_toe_roll_001_cnt_grp|rhino_rig_030:lf_ft_toe_roll_001_cnt_Shift|rhino_rig_030:lf_ft_toe_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt|rhino_rig_030:lf_ft_left_roll_001_cnt_grp|rhino_rig_030:lf_ft_left_roll_001_cnt_Shift|rhino_rig_030:lf_ft_left_roll_001_cnt|rhino_rig_030:lf_ft_right_roll_001_cnt_grp|rhino_rig_030:lf_ft_right_roll_001_cnt_Shift|rhino_rig_030:lf_ft_right_roll_001_cnt|rhino_rig_030:lf_ft_toe_roll_001_cnt_grp|rhino_rig_030:lf_ft_toe_roll_001_cnt_Shift|rhino_rig_030:lf_ft_toe_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt|rhino_rig_030:lf_ft_left_roll_001_cnt_grp|rhino_rig_030:lf_ft_left_roll_001_cnt_Shift|rhino_rig_030:lf_ft_left_roll_001_cnt|rhino_rig_030:lf_ft_right_roll_001_cnt_grp|rhino_rig_030:lf_ft_right_roll_001_cnt_Shift|rhino_rig_030:lf_ft_right_roll_001_cnt|rhino_rig_030:lf_ft_toe_roll_001_cnt_grp|rhino_rig_030:lf_ft_toe_roll_001_cnt_Shift|rhino_rig_030:lf_ft_toe_roll_001_cnt|rhino_rig_030:lf_ft_heel_roll_001_cnt_grp|rhino_rig_030:lf_ft_heel_roll_001_cnt_Shift|rhino_rig_030:lf_ft_heel_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt|rhino_rig_030:lf_ft_left_roll_001_cnt_grp|rhino_rig_030:lf_ft_left_roll_001_cnt_Shift|rhino_rig_030:lf_ft_left_roll_001_cnt|rhino_rig_030:lf_ft_right_roll_001_cnt_grp|rhino_rig_030:lf_ft_right_roll_001_cnt_Shift|rhino_rig_030:lf_ft_right_roll_001_cnt|rhino_rig_030:lf_ft_toe_roll_001_cnt_grp|rhino_rig_030:lf_ft_toe_roll_001_cnt_Shift|rhino_rig_030:lf_ft_toe_roll_001_cnt|rhino_rig_030:lf_ft_heel_roll_001_cnt_grp|rhino_rig_030:lf_ft_heel_roll_001_cnt_Shift|rhino_rig_030:lf_ft_heel_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_main_001_cnt_grp|rhino_rig_030:rt_ft_scapular_main_001_cnt_Shift|rhino_rig_030:rt_ft_scapular_main_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_main_001_cnt_grp|rhino_rig_030:rt_ft_scapular_main_001_cnt_Shift|rhino_rig_030:rt_ft_scapular_main_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_leg_vector_001_cnt_grp|rhino_rig_030:rt_ft_leg_vector_001_cnt_Shift|rhino_rig_030:rt_ft_leg_vector_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_leg_vector_001_cnt_grp|rhino_rig_030:rt_ft_leg_vector_001_cnt_Shift|rhino_rig_030:rt_ft_leg_vector_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt|rhino_rig_030:rt_ft_left_roll_001_cnt_grp|rhino_rig_030:rt_ft_left_roll_001_cnt_Shift|rhino_rig_030:rt_ft_left_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt|rhino_rig_030:rt_ft_left_roll_001_cnt_grp|rhino_rig_030:rt_ft_left_roll_001_cnt_Shift|rhino_rig_030:rt_ft_left_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt|rhino_rig_030:rt_ft_left_roll_001_cnt_grp|rhino_rig_030:rt_ft_left_roll_001_cnt_Shift|rhino_rig_030:rt_ft_left_roll_001_cnt|rhino_rig_030:rt_ft_right_roll_001_cnt_grp|rhino_rig_030:rt_ft_right_roll_001_cnt_Shift|rhino_rig_030:rt_ft_right_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt|rhino_rig_030:rt_ft_left_roll_001_cnt_grp|rhino_rig_030:rt_ft_left_roll_001_cnt_Shift|rhino_rig_030:rt_ft_left_roll_001_cnt|rhino_rig_030:rt_ft_right_roll_001_cnt_grp|rhino_rig_030:rt_ft_right_roll_001_cnt_Shift|rhino_rig_030:rt_ft_right_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt|rhino_rig_030:rt_ft_left_roll_001_cnt_grp|rhino_rig_030:rt_ft_left_roll_001_cnt_Shift|rhino_rig_030:rt_ft_left_roll_001_cnt|rhino_rig_030:rt_ft_right_roll_001_cnt_grp|rhino_rig_030:rt_ft_right_roll_001_cnt_Shift|rhino_rig_030:rt_ft_right_roll_001_cnt|rhino_rig_030:rt_ft_toe_roll_001_cnt_grp|rhino_rig_030:rt_ft_toe_roll_001_cnt_Shift|rhino_rig_030:rt_ft_toe_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt|rhino_rig_030:rt_ft_left_roll_001_cnt_grp|rhino_rig_030:rt_ft_left_roll_001_cnt_Shift|rhino_rig_030:rt_ft_left_roll_001_cnt|rhino_rig_030:rt_ft_right_roll_001_cnt_grp|rhino_rig_030:rt_ft_right_roll_001_cnt_Shift|rhino_rig_030:rt_ft_right_roll_001_cnt|rhino_rig_030:rt_ft_toe_roll_001_cnt_grp|rhino_rig_030:rt_ft_toe_roll_001_cnt_Shift|rhino_rig_030:rt_ft_toe_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt|rhino_rig_030:rt_ft_left_roll_001_cnt_grp|rhino_rig_030:rt_ft_left_roll_001_cnt_Shift|rhino_rig_030:rt_ft_left_roll_001_cnt|rhino_rig_030:rt_ft_right_roll_001_cnt_grp|rhino_rig_030:rt_ft_right_roll_001_cnt_Shift|rhino_rig_030:rt_ft_right_roll_001_cnt|rhino_rig_030:rt_ft_toe_roll_001_cnt_grp|rhino_rig_030:rt_ft_toe_roll_001_cnt_Shift|rhino_rig_030:rt_ft_toe_roll_001_cnt|rhino_rig_030:rt_ft_heel_roll_001_cnt_grp|rhino_rig_030:rt_ft_heel_roll_001_cnt_Shift|rhino_rig_030:rt_ft_heel_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt|rhino_rig_030:rt_ft_left_roll_001_cnt_grp|rhino_rig_030:rt_ft_left_roll_001_cnt_Shift|rhino_rig_030:rt_ft_left_roll_001_cnt|rhino_rig_030:rt_ft_right_roll_001_cnt_grp|rhino_rig_030:rt_ft_right_roll_001_cnt_Shift|rhino_rig_030:rt_ft_right_roll_001_cnt|rhino_rig_030:rt_ft_toe_roll_001_cnt_grp|rhino_rig_030:rt_ft_toe_roll_001_cnt_Shift|rhino_rig_030:rt_ft_toe_roll_001_cnt|rhino_rig_030:rt_ft_heel_roll_001_cnt_grp|rhino_rig_030:rt_ft_heel_roll_001_cnt_Shift|rhino_rig_030:rt_ft_heel_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt|rhino_rig_030:lf_bk_left_roll_001_cnt_grp|rhino_rig_030:lf_bk_left_roll_001_cnt_Shift|rhino_rig_030:lf_bk_left_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt|rhino_rig_030:lf_bk_left_roll_001_cnt_grp|rhino_rig_030:lf_bk_left_roll_001_cnt_Shift|rhino_rig_030:lf_bk_left_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt|rhino_rig_030:lf_bk_left_roll_001_cnt_grp|rhino_rig_030:lf_bk_left_roll_001_cnt_Shift|rhino_rig_030:lf_bk_left_roll_001_cnt|rhino_rig_030:lf_bk_right_roll_001_cnt_grp|rhino_rig_030:lf_bk_right_roll_001_cnt_Shift|rhino_rig_030:lf_bk_right_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt|rhino_rig_030:lf_bk_left_roll_001_cnt_grp|rhino_rig_030:lf_bk_left_roll_001_cnt_Shift|rhino_rig_030:lf_bk_left_roll_001_cnt|rhino_rig_030:lf_bk_right_roll_001_cnt_grp|rhino_rig_030:lf_bk_right_roll_001_cnt_Shift|rhino_rig_030:lf_bk_right_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt|rhino_rig_030:lf_bk_left_roll_001_cnt_grp|rhino_rig_030:lf_bk_left_roll_001_cnt_Shift|rhino_rig_030:lf_bk_left_roll_001_cnt|rhino_rig_030:lf_bk_right_roll_001_cnt_grp|rhino_rig_030:lf_bk_right_roll_001_cnt_Shift|rhino_rig_030:lf_bk_right_roll_001_cnt|rhino_rig_030:lf_bk_toe_roll_001_cnt_grp|rhino_rig_030:lf_bk_toe_roll_001_cnt_Shift|rhino_rig_030:lf_bk_toe_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt|rhino_rig_030:lf_bk_left_roll_001_cnt_grp|rhino_rig_030:lf_bk_left_roll_001_cnt_Shift|rhino_rig_030:lf_bk_left_roll_001_cnt|rhino_rig_030:lf_bk_right_roll_001_cnt_grp|rhino_rig_030:lf_bk_right_roll_001_cnt_Shift|rhino_rig_030:lf_bk_right_roll_001_cnt|rhino_rig_030:lf_bk_toe_roll_001_cnt_grp|rhino_rig_030:lf_bk_toe_roll_001_cnt_Shift|rhino_rig_030:lf_bk_toe_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt|rhino_rig_030:lf_bk_left_roll_001_cnt_grp|rhino_rig_030:lf_bk_left_roll_001_cnt_Shift|rhino_rig_030:lf_bk_left_roll_001_cnt|rhino_rig_030:lf_bk_right_roll_001_cnt_grp|rhino_rig_030:lf_bk_right_roll_001_cnt_Shift|rhino_rig_030:lf_bk_right_roll_001_cnt|rhino_rig_030:lf_bk_toe_roll_001_cnt_grp|rhino_rig_030:lf_bk_toe_roll_001_cnt_Shift|rhino_rig_030:lf_bk_toe_roll_001_cnt|rhino_rig_030:lf_bk_heel_roll_001_cnt_grp|rhino_rig_030:lf_bk_heel_roll_001_cnt_Shift|rhino_rig_030:lf_bk_heel_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt|rhino_rig_030:lf_bk_left_roll_001_cnt_grp|rhino_rig_030:lf_bk_left_roll_001_cnt_Shift|rhino_rig_030:lf_bk_left_roll_001_cnt|rhino_rig_030:lf_bk_right_roll_001_cnt_grp|rhino_rig_030:lf_bk_right_roll_001_cnt_Shift|rhino_rig_030:lf_bk_right_roll_001_cnt|rhino_rig_030:lf_bk_toe_roll_001_cnt_grp|rhino_rig_030:lf_bk_toe_roll_001_cnt_Shift|rhino_rig_030:lf_bk_toe_roll_001_cnt|rhino_rig_030:lf_bk_heel_roll_001_cnt_grp|rhino_rig_030:lf_bk_heel_roll_001_cnt_Shift|rhino_rig_030:lf_bk_heel_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_leg_vector_001_cnt_grp_001|rhino_rig_030:lf_bk_leg_vector_001_cnt_Shift_001|rhino_rig_030:lf_bk_leg_vector_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt|rhino_rig_030:rt_bk_left_roll_001_cnt_grp|rhino_rig_030:rt_bk_left_roll_001_cnt_Shift|rhino_rig_030:rt_bk_left_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt|rhino_rig_030:rt_bk_left_roll_001_cnt_grp|rhino_rig_030:rt_bk_left_roll_001_cnt_Shift|rhino_rig_030:rt_bk_left_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt|rhino_rig_030:rt_bk_left_roll_001_cnt_grp|rhino_rig_030:rt_bk_left_roll_001_cnt_Shift|rhino_rig_030:rt_bk_left_roll_001_cnt|rhino_rig_030:rt_bk_right_roll_001_cnt_grp|rhino_rig_030:rt_bk_right_roll_001_cnt_Shift|rhino_rig_030:rt_bk_right_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt|rhino_rig_030:rt_bk_left_roll_001_cnt_grp|rhino_rig_030:rt_bk_left_roll_001_cnt_Shift|rhino_rig_030:rt_bk_left_roll_001_cnt|rhino_rig_030:rt_bk_right_roll_001_cnt_grp|rhino_rig_030:rt_bk_right_roll_001_cnt_Shift|rhino_rig_030:rt_bk_right_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt|rhino_rig_030:rt_bk_left_roll_001_cnt_grp|rhino_rig_030:rt_bk_left_roll_001_cnt_Shift|rhino_rig_030:rt_bk_left_roll_001_cnt|rhino_rig_030:rt_bk_right_roll_001_cnt_grp|rhino_rig_030:rt_bk_right_roll_001_cnt_Shift|rhino_rig_030:rt_bk_right_roll_001_cnt|rhino_rig_030:rt_bk_toe_roll_001_cnt_grp|rhino_rig_030:rt_bk_toe_roll_001_cnt_Shift|rhino_rig_030:rt_bk_toe_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt|rhino_rig_030:rt_bk_left_roll_001_cnt_grp|rhino_rig_030:rt_bk_left_roll_001_cnt_Shift|rhino_rig_030:rt_bk_left_roll_001_cnt|rhino_rig_030:rt_bk_right_roll_001_cnt_grp|rhino_rig_030:rt_bk_right_roll_001_cnt_Shift|rhino_rig_030:rt_bk_right_roll_001_cnt|rhino_rig_030:rt_bk_toe_roll_001_cnt_grp|rhino_rig_030:rt_bk_toe_roll_001_cnt_Shift|rhino_rig_030:rt_bk_toe_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt|rhino_rig_030:rt_bk_left_roll_001_cnt_grp|rhino_rig_030:rt_bk_left_roll_001_cnt_Shift|rhino_rig_030:rt_bk_left_roll_001_cnt|rhino_rig_030:rt_bk_right_roll_001_cnt_grp|rhino_rig_030:rt_bk_right_roll_001_cnt_Shift|rhino_rig_030:rt_bk_right_roll_001_cnt|rhino_rig_030:rt_bk_toe_roll_001_cnt_grp|rhino_rig_030:rt_bk_toe_roll_001_cnt_Shift|rhino_rig_030:rt_bk_toe_roll_001_cnt|rhino_rig_030:rt_bk_heel_roll_001_cnt_grp|rhino_rig_030:rt_bk_heel_roll_001_cnt_Shift|rhino_rig_030:rt_bk_heel_roll_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt|rhino_rig_030:rt_bk_left_roll_001_cnt_grp|rhino_rig_030:rt_bk_left_roll_001_cnt_Shift|rhino_rig_030:rt_bk_left_roll_001_cnt|rhino_rig_030:rt_bk_right_roll_001_cnt_grp|rhino_rig_030:rt_bk_right_roll_001_cnt_Shift|rhino_rig_030:rt_bk_right_roll_001_cnt|rhino_rig_030:rt_bk_toe_roll_001_cnt_grp|rhino_rig_030:rt_bk_toe_roll_001_cnt_Shift|rhino_rig_030:rt_bk_toe_roll_001_cnt|rhino_rig_030:rt_bk_heel_roll_001_cnt_grp|rhino_rig_030:rt_bk_heel_roll_001_cnt_Shift|rhino_rig_030:rt_bk_heel_roll_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_leg_vector_001_cnt_grp|rhino_rig_030:rt_bk_leg_vector_001_cnt_Shift|rhino_rig_030:rt_bk_leg_vector_001_cnt" 
		"translate" " -type \"double3\" 0 0 0"
		2 "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_leg_vector_001_cnt_grp|rhino_rig_030:rt_bk_leg_vector_001_cnt_Shift|rhino_rig_030:rt_bk_leg_vector_001_cnt" 
		"rotate" " -type \"double3\" 0 0 0"
		2 "rhino_rig_030:gound_lyr" "visibility" " 1"
		2 "rhino_rig_030:joint_layer" "visibility" " 0"
		2 "rhino_rig_030:joint_layer" "hideOnPlayback" " 0"
		2 "rhino_rig_030:control_layer" "visibility" " 1"
		2 "rhino_rig_030:control_layer" "hideOnPlayback" " 1"
		2 "rhino_rig_030:model_layer" "displayType" " 0"
		2 "rhino_rig_030:model_layer" "visibility" " 1"
		2 "rhino_rig_030:deltaMush1" "envelope" " 1"
		2 "rhino_rig_030:deltaMush1" "smoothingStep" " 0.5"
		3 "|rhino_rig_030:model_grp|rhino_rig_030:rhino_body_geo|rhino_rig_030:rhino_body_geo_Shape.instObjGroups" 
		"rhino_rig_030:standardSurface2SG.dagSetMembers" "-na"
		5 0 "rhino_rig_029RN" "rhino_rig_030:control_layer.drawInfo" "|rhino_rig_030:rig_grp.drawOverride" 
		"rhino_rig_029RN.placeHolderList[1]" "rhino_rig_029RN.placeHolderList[2]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_neck_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_neck_jnt_001_cnt_grp|rhino_rig_030:cn_neck_jnt_001_cnt_Shift|rhino_rig_030:cn_neck_jnt_001_cnt.WorldSpace" 
		"rhino_rig_029RN.placeHolderList[3]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_neck_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_neck_jnt_001_cnt_grp|rhino_rig_030:cn_neck_jnt_001_cnt_Shift|rhino_rig_030:cn_neck_jnt_001_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[4]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_neck_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_neck_jnt_001_cnt_grp|rhino_rig_030:cn_neck_jnt_001_cnt_Shift|rhino_rig_030:cn_neck_jnt_001_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[5]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_neck_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_neck_jnt_001_cnt_grp|rhino_rig_030:cn_neck_jnt_001_cnt_Shift|rhino_rig_030:cn_neck_jnt_001_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[6]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_chest_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_chest_jnt_001_cnt_grp|rhino_rig_030:cn_chest_jnt_001_cnt_Shift|rhino_rig_030:cn_chest_jnt_001_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[7]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_chest_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_chest_jnt_001_cnt_grp|rhino_rig_030:cn_chest_jnt_001_cnt_Shift|rhino_rig_030:cn_chest_jnt_001_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[8]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_chest_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_chest_jnt_001_cnt_grp|rhino_rig_030:cn_chest_jnt_001_cnt_Shift|rhino_rig_030:cn_chest_jnt_001_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[9]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_chest_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_chest_jnt_001_cnt_grp|rhino_rig_030:cn_chest_jnt_001_cnt_Shift|rhino_rig_030:cn_chest_jnt_001_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[10]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_chest_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_chest_jnt_001_cnt_grp|rhino_rig_030:cn_chest_jnt_001_cnt_Shift|rhino_rig_030:cn_chest_jnt_001_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[11]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_chest_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_chest_jnt_001_cnt_grp|rhino_rig_030:cn_chest_jnt_001_cnt_Shift|rhino_rig_030:cn_chest_jnt_001_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[12]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_pelvis_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_Shift|rhino_rig_030:cn_pelvis_jnt_002_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[13]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_pelvis_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_Shift|rhino_rig_030:cn_pelvis_jnt_002_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[14]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_pelvis_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_Shift|rhino_rig_030:cn_pelvis_jnt_002_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[15]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_pelvis_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_Shift|rhino_rig_030:cn_pelvis_jnt_002_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[16]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_pelvis_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_Shift|rhino_rig_030:cn_pelvis_jnt_002_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[17]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_body_grp|rhino_rig_030:cn_pelvis_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_grp|rhino_rig_030:cn_pelvis_jnt_002_cnt_Shift|rhino_rig_030:cn_pelvis_jnt_002_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[18]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_head_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_Shift|rhino_rig_030:cn_jaw_jnt_001_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[19]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_head_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_Shift|rhino_rig_030:cn_jaw_jnt_001_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[20]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_head_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_Shift|rhino_rig_030:cn_jaw_jnt_001_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[21]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_head_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_Shift|rhino_rig_030:cn_jaw_jnt_001_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[22]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_head_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_Shift|rhino_rig_030:cn_jaw_jnt_001_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[23]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_head_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_grp|rhino_rig_030:cn_jaw_jnt_001_cnt_Shift|rhino_rig_030:cn_jaw_jnt_001_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[24]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_001_cnt_grp|rhino_rig_030:lf_ft_scapular_001_cnt_Shift|rhino_rig_030:lf_ft_scapular_001_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[25]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_001_cnt_grp|rhino_rig_030:lf_ft_scapular_001_cnt_Shift|rhino_rig_030:lf_ft_scapular_001_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[26]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_001_cnt_grp|rhino_rig_030:lf_ft_scapular_001_cnt_Shift|rhino_rig_030:lf_ft_scapular_001_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[27]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_001_cnt_grp|rhino_rig_030:lf_ft_scapular_001_cnt_Shift|rhino_rig_030:lf_ft_scapular_001_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[28]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_001_cnt_grp|rhino_rig_030:lf_ft_scapular_001_cnt_Shift|rhino_rig_030:lf_ft_scapular_001_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[29]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_001_cnt_grp|rhino_rig_030:lf_ft_scapular_001_cnt_Shift|rhino_rig_030:lf_ft_scapular_001_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[30]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_002_cnt_grp|rhino_rig_030:lf_ft_scapular_002_cnt_Shift|rhino_rig_030:lf_ft_scapular_002_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[31]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_002_cnt_grp|rhino_rig_030:lf_ft_scapular_002_cnt_Shift|rhino_rig_030:lf_ft_scapular_002_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[32]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_002_cnt_grp|rhino_rig_030:lf_ft_scapular_002_cnt_Shift|rhino_rig_030:lf_ft_scapular_002_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[33]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_002_cnt_grp|rhino_rig_030:lf_ft_scapular_002_cnt_Shift|rhino_rig_030:lf_ft_scapular_002_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[34]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_002_cnt_grp|rhino_rig_030:lf_ft_scapular_002_cnt_Shift|rhino_rig_030:lf_ft_scapular_002_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[35]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_002_cnt_grp|rhino_rig_030:lf_ft_scapular_002_cnt_Shift|rhino_rig_030:lf_ft_scapular_002_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[36]" ""
		5 3 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_002_cnt_grp|rhino_rig_030:lf_ft_scapular_002_cnt_Shift|rhino_rig_030:lf_ft_scapular_002_cnt.message" 
		"rhino_rig_029RN.placeHolderList[37]" ""
		5 3 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_scapular_002_cnt_grp|rhino_rig_030:lf_ft_scapular_002_cnt_Shift|rhino_rig_030:lf_ft_scapular_002_cnt|rhino_rig_030:lf_ft_scapular_002_cntShape.message" 
		"rhino_rig_029RN.placeHolderList[38]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[39]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[40]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[41]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[42]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[43]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_ft_foot_001_cnt_grp|rhino_rig_030:lf_ft_foot_001_cnt_Shift|rhino_rig_030:lf_ft_foot_001_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[44]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_001_cnt_grp|rhino_rig_030:rt_ft_scapular_001_cnt_Shift|rhino_rig_030:rt_ft_scapular_001_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[45]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_001_cnt_grp|rhino_rig_030:rt_ft_scapular_001_cnt_Shift|rhino_rig_030:rt_ft_scapular_001_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[46]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_001_cnt_grp|rhino_rig_030:rt_ft_scapular_001_cnt_Shift|rhino_rig_030:rt_ft_scapular_001_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[47]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_001_cnt_grp|rhino_rig_030:rt_ft_scapular_001_cnt_Shift|rhino_rig_030:rt_ft_scapular_001_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[48]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_001_cnt_grp|rhino_rig_030:rt_ft_scapular_001_cnt_Shift|rhino_rig_030:rt_ft_scapular_001_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[49]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_001_cnt_grp|rhino_rig_030:rt_ft_scapular_001_cnt_Shift|rhino_rig_030:rt_ft_scapular_001_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[50]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_002_cnt_grp|rhino_rig_030:rt_ft_scapular_002_cnt_Shift|rhino_rig_030:rt_ft_scapular_002_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[51]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_002_cnt_grp|rhino_rig_030:rt_ft_scapular_002_cnt_Shift|rhino_rig_030:rt_ft_scapular_002_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[52]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_002_cnt_grp|rhino_rig_030:rt_ft_scapular_002_cnt_Shift|rhino_rig_030:rt_ft_scapular_002_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[53]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_002_cnt_grp|rhino_rig_030:rt_ft_scapular_002_cnt_Shift|rhino_rig_030:rt_ft_scapular_002_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[54]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_002_cnt_grp|rhino_rig_030:rt_ft_scapular_002_cnt_Shift|rhino_rig_030:rt_ft_scapular_002_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[55]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_scapular_002_cnt_grp|rhino_rig_030:rt_ft_scapular_002_cnt_Shift|rhino_rig_030:rt_ft_scapular_002_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[56]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[57]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[58]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[59]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[60]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[61]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_ft_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_ft_foot_001_cnt_grp|rhino_rig_030:rt_ft_foot_001_cnt_Shift|rhino_rig_030:rt_ft_foot_001_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[62]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[63]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[64]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[65]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[66]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[67]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_foot_001_cnt_grp|rhino_rig_030:lf_bk_foot_001_cnt_Shift|rhino_rig_030:lf_bk_foot_001_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[68]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_hip_ankle_cnt_grp|rhino_rig_030:lf_bk_hip_ankle_cnt_Shift|rhino_rig_030:lf_bk_hip_ankle_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[69]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_hip_ankle_cnt_grp|rhino_rig_030:lf_bk_hip_ankle_cnt_Shift|rhino_rig_030:lf_bk_hip_ankle_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[70]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:lf_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:lf_bk_hip_ankle_cnt_grp|rhino_rig_030:lf_bk_hip_ankle_cnt_Shift|rhino_rig_030:lf_bk_hip_ankle_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[71]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[72]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[73]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[74]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[75]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[76]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_foot_001_cnt_grp|rhino_rig_030:rt_bk_foot_001_cnt_Shift|rhino_rig_030:rt_bk_foot_001_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[77]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_Shift|rhino_rig_030:rt_bk_hip_ankle_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[78]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_Shift|rhino_rig_030:rt_bk_hip_ankle_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[79]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_Shift|rhino_rig_030:rt_bk_hip_ankle_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[80]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_Shift|rhino_rig_030:rt_bk_hip_ankle_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[81]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_Shift|rhino_rig_030:rt_bk_hip_ankle_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[82]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_legs_grp|rhino_rig_030:rt_bk_leg_grp|rhino_rig_030:control_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_grp|rhino_rig_030:rt_bk_hip_ankle_cnt_Shift|rhino_rig_030:rt_bk_hip_ankle_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[83]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt.worldspace" 
		"rhino_rig_029RN.placeHolderList[84]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[85]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[86]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[87]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[88]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[89]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[90]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[91]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[92]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[93]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[94]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[95]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[96]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[97]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[98]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[99]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[100]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[101]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[102]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[103]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[104]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[105]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[106]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[107]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[108]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt|rhino_rig_030:cn_tail_jnt_005_cnt_grp|rhino_rig_030:cn_tail_jnt_005_cnt_Shift|rhino_rig_030:cn_tail_jnt_005_cnt.translateX" 
		"rhino_rig_029RN.placeHolderList[109]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt|rhino_rig_030:cn_tail_jnt_005_cnt_grp|rhino_rig_030:cn_tail_jnt_005_cnt_Shift|rhino_rig_030:cn_tail_jnt_005_cnt.translateY" 
		"rhino_rig_029RN.placeHolderList[110]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt|rhino_rig_030:cn_tail_jnt_005_cnt_grp|rhino_rig_030:cn_tail_jnt_005_cnt_Shift|rhino_rig_030:cn_tail_jnt_005_cnt.translateZ" 
		"rhino_rig_029RN.placeHolderList[111]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt|rhino_rig_030:cn_tail_jnt_005_cnt_grp|rhino_rig_030:cn_tail_jnt_005_cnt_Shift|rhino_rig_030:cn_tail_jnt_005_cnt.rotateX" 
		"rhino_rig_029RN.placeHolderList[112]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt|rhino_rig_030:cn_tail_jnt_005_cnt_grp|rhino_rig_030:cn_tail_jnt_005_cnt_Shift|rhino_rig_030:cn_tail_jnt_005_cnt.rotateY" 
		"rhino_rig_029RN.placeHolderList[113]" ""
		5 4 "rhino_rig_029RN" "|rhino_rig_030:rig_grp|rhino_rig_030:world_cnt_001_grp|rhino_rig_030:world_cnt_001_Shift|rhino_rig_030:world_cnt_001|rhino_rig_030:world_cnt_002_grp|rhino_rig_030:world_cnt_002_Shift|rhino_rig_030:world_cnt_002_cnt|rhino_rig_030:rig_setup_grp|rhino_rig_030:cn_tail_grp|rhino_rig_030:control_grp|rhino_rig_030:cn_tail_jnt_001_cnt_grp|rhino_rig_030:cn_tail_jnt_001_cnt_Shift|rhino_rig_030:cn_tail_jnt_001_cnt|rhino_rig_030:cn_tail_jnt_002_cnt_grp|rhino_rig_030:cn_tail_jnt_002_cnt_Shift|rhino_rig_030:cn_tail_jnt_002_cnt|rhino_rig_030:cn_tail_jnt_003_cnt_grp|rhino_rig_030:cn_tail_jnt_003_cnt_Shift|rhino_rig_030:cn_tail_jnt_003_cnt|rhino_rig_030:cn_tail_jnt_004_cnt_grp|rhino_rig_030:cn_tail_jnt_004_cnt_Shift|rhino_rig_030:cn_tail_jnt_004_cnt|rhino_rig_030:cn_tail_jnt_005_cnt_grp|rhino_rig_030:cn_tail_jnt_005_cnt_Shift|rhino_rig_030:cn_tail_jnt_005_cnt.rotateZ" 
		"rhino_rig_029RN.placeHolderList[114]" ""
		5 0 "rhino_rig_029RN" "|rhino_rig_030:model_grp|rhino_rig_030:rhino_body_geo|rhino_rig_030:rhino_body_geo_Shape.instObjGroups" 
		"rhino_rig_030:standardSurface2SG.dagSetMembers" "rhino_rig_029RN.placeHolderList[115]" 
		"rhino_rig_029RN.placeHolderList[116]" "rhino_rig_030:standardSurface2SG.dsm";
	setAttr ".ptag" -type "string" "";
lockNode -l 1 ;
createNode objectSet -s -n "lightEditorRoot";
	rename -uid "92F0BBAF-4BED-2ABC-A77A-4DB04BC4E585";
	addAttr -ci true -sn "isolate" -ln "isolate" -min 0 -max 1 -at "bool";
	addAttr -ci true -sn "wasEnabled" -ln "wasEnabled" -dv 1 -min 0 -max 1 -at "bool";
	addAttr -ci true -sn "childIndex" -ln "childIndex" -dv -1 -at "long";
	addAttr -ci true -sn "lightGroup" -ln "lightGroup" -dv 1 -min 0 -max 1 -at "bool";
	addAttr -ci true -sn "visibility" -ln "visibility" -dv 1 -min 0 -max 1 -at "bool";
lockNode -l 1 ;
createNode reference -n "sharedReferenceNode";
	rename -uid "AF654F07-419C-1C56-3AC4-9DB16B16A333";
	setAttr ".ed" -type "dataReferenceEdits" 
		"sharedReferenceNode";
createNode animCurveTL -n "lf_ft_scapular_002_cnt_translateX";
	rename -uid "0FF06D38-45FC-BFE5-DA9F-488BC54574AB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 6 ".ktv[0:5]"  2 0 13 0.073889406051018708 33 0.73826296479923115
		 42 0.63846137146952175 50 0.2536569659127661 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_ft_scapular_002_cnt_translateY";
	rename -uid "359D0239-4B1C-C96E-B95B-B78FCA62EE28";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 5 ".ktv[0:4]"  2 0 13 -8.2837194446654454 33 -11.632645711582096
		 42 -10.060094152028213 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_ft_scapular_002_cnt_translateZ";
	rename -uid "541662CD-4810-668A-0E2E-FBAE0F12780C";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 6 ".ktv[0:5]"  2 0 13 -0.043603609073885274 33 2.06354321196546
		 42 1.7845844800793691 50 -1.0748040343285319 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_ft_scapular_002_cnt_rotateX";
	rename -uid "8CF3CFE1-466A-8605-ABF8-0AA8C3A7B221";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 5 ".ktv[0:4]"  2 0 13 0 33 0 42 0 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_ft_scapular_002_cnt_rotateY";
	rename -uid "D17E9DB2-480D-84E9-F44B-2289037BBEBB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 5 ".ktv[0:4]"  2 0 13 0 33 0 42 0 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_ft_scapular_002_cnt_rotateZ";
	rename -uid "A5FDD373-49A9-B200-6DE7-F69D1ACA8D7E";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 5 ".ktv[0:4]"  2 0 13 0 33 0 42 0 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_ft_scapular_002_cnt_translateX";
	rename -uid "27C6ABF7-458E-5470-68C6-999CC63156D6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 5 ".ktv[0:4]"  2 0 11 0.85756101637546567 36 0.76183360677923329
		 48 0.91638452730429376 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_ft_scapular_002_cnt_translateY";
	rename -uid "F3CBA57E-40A1-8015-959B-EFB81D793A5A";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 6 ".ktv[0:5]"  2 0 11 -12.482668032265932 28 -20.758761145906021
		 36 -20.758761145906021 48 -5.5260579504243736 72 0;
	setAttr -s 6 ".kit[3:5]"  1 18 18;
	setAttr -s 6 ".kot[3:5]"  1 18 18;
	setAttr -s 6 ".kix[3:5]"  1 0.072070740344926665 1;
	setAttr -s 6 ".kiy[3:5]"  0 0.99739952295263012 0;
	setAttr -s 6 ".kox[3:5]"  1 0.072070740344926665 1;
	setAttr -s 6 ".koy[3:5]"  0 0.99739952295263012 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_ft_scapular_002_cnt_translateZ";
	rename -uid "9F37F96B-4B72-2CD0-3A81-47A2AEB08FE4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 5 ".ktv[0:4]"  2 0 11 -9.4006685298276 36 9.8338189330850767
		 48 11.828776565382711 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_ft_scapular_002_cnt_rotateX";
	rename -uid "E294BCD1-4683-9634-C54B-8EA595298267";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  2 0 36 0 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_ft_scapular_002_cnt_rotateY";
	rename -uid "304C5FC0-4D9F-ECE1-B295-37A294B362D1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  2 0 36 0 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_ft_scapular_002_cnt_rotateZ";
	rename -uid "8E441F93-453F-B8C7-BE22-3EB322AF056B";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  2 0 36 0 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_bk_hip_ankle_cnt_rotateX";
	rename -uid "0CC3D121-4C34-B784-2434-6598C5238300";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 22 ".ktv[0:21]"  17 0 23 0.26356138794702549 26 0.69621814787034952
		 29 1.4211967517509181 32 2.5073005495817595 35 4.0233328913559019 38 7.2109756505666676
		 41 16.813669010929097 44 20.891262155894527 47 23.776561932827747 50 25.266727413248457
		 53 25.88326563936112 56 26.30701015307373 62 26.72071332759884 68 26.264274750848585
		 71 23.768791733492193 74 19.819155339883658 77 15.06906890689207 80 10.172235771386509
		 83 5.7823592702360624 86 2.1061015569265025 87 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_bk_hip_ankle_cnt_rotateY";
	rename -uid "9A6799BE-4E89-8414-C7A3-1E8F35E9F390";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0 79 0 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_bk_hip_ankle_cnt_rotateZ";
	rename -uid "997D32FD-4C14-C59F-63E1-EE88096E3ABA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0 79 0 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_bk_hip_ankle_cnt_translateX";
	rename -uid "55D70F76-4713-695C-4325-02B9FBE72A06";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0.0051975894802250705
		 55 0.022489206068876312 56 0.054423121987917558 57 0.1035476094593132 58 0.1724109407050271
		 59 0.26356138794702283 60 0.37954722340726527 61 0.52291671930771821 62 0.69621814787034464
		 63 0.90199978131711045 64 1.1428098918699772 65 1.4211967517509136 66 1.7397086331818772
		 67 2.1008938083848352 68 2.5073005495817555 69 2.9614771289945945 70 3.4659718188453197
		 71 4.023332891355901 72 4.6361086187482909 73 5.3068472732444594 74 7.2109756505666827
		 75 10.642855215455352 76 14.283436243659731 77 16.813669010929097 78 18.292822721842096
		 79 19.654524645824143 80 20.891262155894495 81 21.995522625072368 82 22.959793426377033
		 83 23.776561932827732 84 24.438315517443698 85 24.937541553244191 86 25.266727413248457
		 87 25.49589308759435 88 25.700959547890236 89 25.883265639361117 90 26.04415020723199
		 91 26.184952096727862 92 26.307010153073726 93 26.411663221494589 94 26.500250147215446
		 95 26.574109775461299 96 26.634580951457149 97 26.683002520427994 98 26.72071332759884
		 99 26.749052218194681 100 26.769358037440519 101 26.782969630561357 102 26.791225842782193
		 103 26.79546551932803 104 26.797027505423863 105 26.797250646294696 106 26.659971466650028
		 107 26.264274750848589 108 25.634371733589244 109 24.794473649570843 110 23.768791733492225
		 111 22.581537220052272 112 21.256921343949809 113 19.819155339883736 114 18.292450442552848
		 115 16.701017886656029 116 15.069068906892166 117 13.420814737960047 118 11.780466614558559
		 119 10.172235771386596 120 8.6203334431429361 121 7.1489708645264791 122 5.7823592702360997
		 123 4.5447098949705991 124 3.3263974392141229 125 2.1061015569264891 126 1.0404909130200042
		 127 0.28623417240705429 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0 136 0 137 0
		 138 0 139 0 140 0 141 0 142 0 143 0 144 0 145 0 146 0 147 0 148 0 149 0 150 0 151 0
		 152 0 153 0 154 0 155 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_bk_hip_ankle_cnt_translateY";
	rename -uid "C783BA2F-4CC5-6B7E-D5F7-AE9CDFF89E55";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0 79 0 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0 88 0 89 0 90 0 91 0 92 0
		 93 0 94 0 95 0 96 0 97 0 98 0 99 0 100 0 101 0 102 0 103 0 104 0 105 0 106 0 107 0
		 108 0 109 0 110 0 111 0 112 0 113 0 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0
		 122 0 123 0 124 0 125 0 126 0 127 0 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0
		 136 0 137 0 138 0 139 0 140 0 141 0 142 0 143 0 144 0 145 0 146 0 147 0 148 0 149 0
		 150 0 151 0 152 0 153 0 154 0 155 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_bk_hip_ankle_cnt_translateZ";
	rename -uid "5FCB5299-43A5-5546-004C-21AFA69C3BB9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0 79 0 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0 88 0 89 0 90 0 91 0 92 0
		 93 0 94 0 95 0 96 0 97 0 98 0 99 0 100 0 101 0 102 0 103 0 104 0 105 0 106 0 107 0
		 108 0 109 0 110 0 111 0 112 0 113 0 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0
		 122 0 123 0 124 0 125 0 126 0 127 0 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0
		 136 0 137 0 138 0 139 0 140 0 141 0 142 0 143 0 144 0 145 0 146 0 147 0 148 0 149 0
		 150 0 151 0 152 0 153 0 154 0 155 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_bk_hip_ankle_cnt_rotateX";
	rename -uid "F6293083-4B10-B8EE-F479-CAB4903293FA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0 79 0 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0 88 0 89 0 90 0 91 0 92 0
		 93 0 94 0 95 0 96 0 97 0 98 0 99 0 100 0 101 0 102 0 103 0 104 0 105 0 106 0 107 0
		 108 0 109 0 110 0 111 0 112 0 113 0 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0
		 122 0 123 0 124 0 125 0 126 0 127 0 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0
		 136 0 137 0 138 0 139 0 140 0 141 0 142 0 143 0 144 0 145 0 146 0 147 0 148 0 149 0
		 150 0 151 0 152 0 153 0 154 0 155 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_bk_hip_ankle_cnt_rotateY";
	rename -uid "B508684B-4050-7393-5E0A-CFB85A89B9F6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0 79 0 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0 88 0 89 0 90 0 91 0 92 0
		 93 0 94 0 95 0 96 0 97 0 98 0 99 0 100 0 101 0 102 0 103 0 104 0 105 0 106 0 107 0
		 108 0 109 0 110 0 111 0 112 0 113 0 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0
		 122 0 123 0 124 0 125 0 126 0 127 0 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0
		 136 0 137 0 138 0 139 0 140 0 141 0 142 0 143 0 144 0 145 0 146 0 147 0 148 0 149 0
		 150 0 151 0 152 0 153 0 154 0 155 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_bk_hip_ankle_cnt_rotateZ";
	rename -uid "3807D757-4631-8D34-F054-3C938935342C";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0 79 0 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0 88 0 89 0 90 0 91 0 92 0
		 93 0 94 0 95 0 96 0 97 0 98 0 99 0 100 0 101 0 102 0 103 0 104 0 105 0 106 0 107 0
		 108 0 109 0 110 0 111 0 112 0 113 0 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0
		 122 0 123 0 124 0 125 0 126 0 127 0 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0
		 136 0 137 0 138 0 139 0 140 0 141 0 142 0 143 0 144 0 145 0 146 0 147 0 148 0 149 0
		 150 0 151 0 152 0 153 0 154 0 155 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_chest_jnt_001_cnt_translateX";
	rename -uid "DBC6BB51-45E0-753C-1B1C-AB95E86C0DF0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 0.28746073571401176 3 0.3464656971132456
		 4 0.39401269090392255 5 0.42466086103155476 6 0.4330049723130287 7 0.425774754626838
		 8 0.41359477198069783 9 0.39696452026738527 10 0.37642630046850378 11 0.35256244734680742
		 12 0.32599074175040244 13 0.29735801383814575 14 0.26733194428255835 15 0.23659106904224281
		 16 0.2058129905406787 17 0.17566079410330282 18 0.14676766352247306 19 0.11948838710849685
		 20 0.093960602944312299 21 0.070435066670409441 22 0.049136209688246169 23 0.030269065999481981
		 24 0.014025721148481551 25 0.00059128775816930101 26 -0.0098505893089910757 27 -0.017112696588775123
		 28 -0.020999684229426663 29 -0.021303967736225005 30 -0.017802101146076055 31 -0.003963771706736452
		 32 0.024458134874427273 33 0.06429432188540396 34 0.11240109096031325 35 0.16566156888336003
		 36 0.2209864465300253 37 0.27531422853246923 38 0.32561099303300267 39 0.36886966170173885
		 40 0.40210878098488756 41 0.42237081626267603 42 0.42671996117861966 43 0.41971665086855658
		 44 0.40823789639854269 45 0.39284613375080113 46 0.37411197091726045 47 0.35260961314108741
		 48 0.32891181712525075 49 0.30358437423454099 50 0.27718012150350368 51 0.25023247795917314
		 52 0.22324850247466088 53 0.1976796029309984 54 0.17446223067116051 55 0.15318880581993355
		 56 0.13338830753436426 57 0.11462542254274588 58 0.096577815684780077 59 0.079091486279109446
		 60 0.062214121905494579 61 0.046206352564642117 62 0.031530833837594585 63 0.018819131339931516
		 64 0.0088164218399171546 65 0.0023040525601327744 66 3.5527136788005009e-15 67 0.012533263354280066
		 68 0.046516966066775467 69 0.09649427673770461 70 0.15697836862661596 71 0.2224679100846032
		 72 0.28746073571401176;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_chest_jnt_001_cnt_translateY";
	rename -uid "6556661C-4F9A-33BA-7035-87BCBEC4B8F3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 -2.0967881227445417 3 -2.5370388533096389
		 4 -2.8997433671663941 5 -3.1460885645876289 6 -3.237256401762977 7 -3.2215632540252415
		 8 -3.1750719471330342 9 -3.1005754408713955 10 -3.0008607537918266 11 -2.8787093478623547
		 12 -2.7368977652277096 13 -2.5781985160647736 14 -2.4053812165540052 15 -2.2212139761907821
		 16 -2.0284650340426822 17 -1.8299046441122186 18 -1.6283072106560326 19 -1.4264857784906724
		 20 -1.2272835323124553 21 -1.0335279563986006 22 -0.8480501887613201 23 -0.67368405972095502
		 24 -0.513265196809499 25 -0.3696301953822001 26 -0.24561585451550627 27 -0.14405847797338822
		 28 -0.067793240210619388 29 -0.019653617545770885 30 -0.002470884767348025 31 -0.064376067921514846
		 32 -0.2368910422217283 33 -0.49792890705599291 34 -0.82539920882912554 35 -1.1972077706858215
		 36 -1.5912565899474913 37 -1.9854438034592476 38 -2.3576637209357472 39 -2.6858069262805486
		 40 -2.9477604467457894 41 -3.1214079896985396 42 -3.1846302466801859 43 -3.1691765028621717
		 44 -3.123369601764665 45 -3.0499473275856275 46 -2.9516463302694831 47 -2.83120276047174
		 48 -2.6913529699014731 49 -2.5348342770383141 50 -2.3643857983888523 51 -2.1827493456279967
		 52 -1.992670389150561 53 -1.7967633261955882 54 -1.5977140286136091 55 -1.3983949162495861
		 56 -1.2016872143877606 57 -1.0104671921955202 58 -0.82759543777575573 59 -0.65590917484901468
		 60 -0.49821763333618208 61 -0.35730048731073794 62 -0.2359093702275743 63 -0.1367724712735594
		 64 -0.062602210706614869 65 -0.016105988420875406 66 0 67 -0.090586393897382322 68 -0.33646862817806777
		 69 -0.6988358204507108 70 -1.1388812515982636 71 -1.6178002157467972 72 -2.0967881227445417;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_chest_jnt_001_cnt_translateZ";
	rename -uid "978BD1CB-41BE-3FC6-6E7A-109A9B267993";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 -0.28214719843207359 3 -0.36979336398011808
		 4 -0.46439091232024687 5 -0.56419266160644388 6 -0.66745152012425768 7 -0.77242116928266991
		 8 -0.87735680740240807 9 -0.98051587965439235 10 -1.080158714128914 11 -1.1745489841171637
		 12 -1.2619539222448513 13 -1.3406442230859501 14 -1.4088935872703003 15 -1.4649778818493695
		 16 -1.5071739187753432 17 -1.5337578857713514 18 -1.5430035016404957 19 -1.5355762116528748
		 20 -1.5138866598076053 21 -1.4788210254664609 22 -1.431263185698354 23 -1.3720956585908466
		 24 -1.3022005093454534 25 -1.2224601785173586 26 -1.1337582012618501 27 -1.0369797952154458
		 28 -0.93301230268224022 29 -0.82274548012382154 30 -0.70707163456834166 31 -0.58688561246095705
		 32 -0.46308465167060875 33 -0.33656811184124003 34 -0.20823710202161635 35 -0.078994027518976578
		 36 0.050257919809928768 37 0.17861530209795129 38 0.30517498033989288 39 0.42903463709397238
		 40 0.54929325125890105 41 0.66505150067960539 42 0.77541207059297779 43 0.87947984892937858
		 44 0.97636199322894113 45 1.0651678584064472 46 1.1450087797977364 47 1.2149977118384025
		 48 1.274248729358485 49 1.3218764058243568 50 1.3569950909242749 51 1.3787181186841611
		 52 1.3861569868261399 53 1.3659939326422965 54 1.3095270743461185 55 1.2227817900252678
		 56 1.1117819488938094 57 0.98255893448918463 58 0.84115687476462786 59 0.69363400918626905
		 60 0.54606081151421437 61 0.4045159479322129 62 0.27508138220390732 63 0.16383794121382358
		 64 0.076862424882278013 65 0.020226884132323132 66 0 67 -0.0092895544230892219 68 -0.035996880967708894
		 69 -0.078379672612142404 70 -0.13469501540019518 71 -0.20319912925191888 72 -0.28214719843207359;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_chest_jnt_001_cnt_rotateX";
	rename -uid "930274B9-45F5-AED7-6769-48A89E868148";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 -0.13476113692645866 3 -0.17664350506127782
		 4 -0.22186533755737115 5 -0.26959969093737629 6 -0.31901967810842435 7 -0.36929784651405828
		 8 -0.41960549059522917 9 -0.46911196767103464 10 -0.51698409185596295 11 -0.56238568130805033
		 12 -0.60447732983971558 13 -0.64241646453416812 14 -0.67535773626229467 15 -0.70245376965114914
		 16 -0.7228562729312249 17 -0.73571747611514549 18 -0.74019182823769381 19 -0.73659738640157302
		 20 -0.72610332071386874 21 -0.70914559662360654 22 -0.68616225794002528 23 -0.65759255327788368
		 24 -0.62387609746067152 25 -0.58545210769652867 26 -0.54275874459248186 27 -0.49623257904162754
		 28 -0.4463081977981439 29 -0.39341795320616318 30 -0.33799185609718918 31 -0.28045760531405889
		 32 -0.22124074263062421 33 -0.16076491797147585 34 -0.099452246741028608 35 -0.037723738688918068
		 36 0.024000223985331588 37 0.085299382696964077 38 0.14575309665126712 39 0.20493985904297735
		 40 0.26243685860685756 41 0.31781960868650694 42 0.37066166422479119 43 0.42053444466920586
		 44 0.46700717767765582 45 0.50964697464961584 46 0.54801904444210259 47 0.58168704611611988
		 48 0.61021357517001118 49 0.63316077044673169 50 0.65009102077541125 51 0.660567741477744
		 52 0.66415618122197051 53 0.65443063279415958 54 0.62720934500409709 55 0.58543342488593686
		 56 0.53204559667312201 57 0.46998129200470884 58 0.40216345094210987 59 0.33150107466891798
		 60 0.26089091381316937 61 0.19322125342008592 62 0.13137655612579341 63 0.078241738602638028
		 64 0.03670507717315416 65 0.0096591669950821923 66 0 67 -0.0044361565126495497 68 -0.017190131472686047
		 69 -0.037430343072251682 70 -0.064325665543767188 71 -0.097045730367578228 72 -0.13476113692645866;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_chest_jnt_001_cnt_rotateY";
	rename -uid "1D19271C-452E-8A65-DA9A-F584257C3334";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 0.97082616560757484 3 1.2724606128907061
		 4 1.5980652415302554 5 1.941646956233869 6 2.2972126577930272 7 2.6587692862380115
		 8 3.020323868552532 9 3.3758835661522588 10 3.7194557169490898 11 4.0450478667756569
		 12 4.3466677852399966 13 4.6183234617314035 14 4.8540230783217639 15 5.0477749577184099
		 16 5.1935874862379068 17 5.2854690139897711 18 5.3174277370781997 19 5.2917541673114581
		 20 5.2167872425731074 21 5.0956074686617736 22 4.9312952071315763 23 4.7269307359228199
		 24 4.4855943075657532 25 4.2103662021935699 26 3.9043267732777185 27 3.5705564846257265
		 28 3.2121359377525178 29 2.8321458892463425 30 2.4336672581983456 31 2.0197811241503989
		 32 1.5935687163410761 33 1.1581113952978372 34 0.7164906280379858 35 0.27178795830613867
		 36 -0.17291502660459004 37 -0.61453673082345417 38 -1.0499955850039331 39 -1.4762100798974138
		 40 -1.8900987967801541 41 -2.288580433195464 42 -2.6685738225951963 43 -3.0269979466317118
		 44 -3.3607719390670838 45 -3.6668150805339512 46 -3.9420467837061586 47 -4.1833865688199872
		 48 -4.3877540299302478 49 -4.552068792790104 50 -4.6732504658078016 51 -4.7482185861535733
		 52 -4.773892563759615 53 -4.704305344832262 54 -4.5094604206057065 55 -4.2102323297846445
		 56 -3.8274954988582297 57 -3.3821248604652174 58 -2.8949962141999763 59 -2.3869863270342839
		 60 -1.8789728161253305 61 -1.3918338861256048 62 -0.94644800694579134 63 -0.56369361697655695
		 64 -0.26444892144733689 65 -0.069591825863919293 66 0 67 0.031961384727851788 68 0.12385036405426239
		 69 0.26967416458562882 70 0.46343998127973962 71 0.69915495654229676 72 0.97082616560757484;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_chest_jnt_001_cnt_rotateZ";
	rename -uid "2AE0BAE6-4983-D827-6875-52872089354C";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 -0.0011417318123404675 3 -0.0019615870633071064
		 4 -0.0030942830094304926 5 -0.0045685606447375977 6 -0.006396248825354855 7 -0.0085700675563182456
		 8 -0.011062278151098746 9 -0.013824181743275602 10 -0.01678647242366518 11 -0.019860455907000911
		 12 -0.022940148331743904 13 -0.025905271816591659 14 -0.028625163080175382 15 -0.030963608287824091
		 16 -0.032784611085151939 17 -0.033959091620886027 18 -0.034372502794274638 19 -0.034040195301616769
		 20 -0.033079192175365243 21 -0.031555180302322197 22 -0.029546673043388972 23 -0.027141552361287222
		 24 -0.024433849413419029 25 -0.021520773892788073 26 -0.018499998960215473 27 -0.01546720518407995
		 28 -0.012513883742279736 29 -0.0097253964297987546 30 -0.0071792878758333983 31 -0.0049438438731363088
		 32 -0.0030768888778962148 33 -0.0016248155307264261 34 -0.00062183942532856426 35 -8.9473235207537992e-05
		 36 -3.6215604485671518e-05 37 -0.00045745180925626209 38 -0.0013355649841048994 39 -0.0026402585636520616
		 40 -0.0043290923888287076 41 -0.0063482365516729488 42 -0.008633448385458483 43 -0.011111278942618609
		 44 -0.013700515747930762 45 -0.016313868494242417 46 -0.018859903611982295 47 -0.021245232273212865
		 48 -0.023376954405307734 49 -0.025165358755371882 50 -0.026526876085116925 51 -0.027387279369137683
		 52 -0.027685121667711105 53 -0.026881610319191698 54 -0.024695229602026185 55 -0.021519402248116472
		 56 -0.017777700220150561 57 -0.013875420214227292 58 -0.010162303539146666 59 -0.0069063132920327095
		 60 -0.0042782533378245228 61 -0.0023469910941956345 62 -0.0010851057676284348 63 -0.00038488638788004755
		 64 -8.470637554621248e-05 65 -5.8660442103410336e-06 66 3.1805546814635168e-15 67 -1.2373137218984886e-06
		 68 -1.8579072522555279e-05 69 -8.8086902534604408e-05 70 -0.00026015224116382089
		 71 -0.00059211046717711207 72 -0.0011417318123404675;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_pelvis_jnt_002_cnt_translateX";
	rename -uid "8EF54BE0-4F16-640D-78F8-53B6F51D1C57";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  17 -0.11187436843644605 18 -0.11085361289602247
		 19 -0.10791957976719857 20 -0.10326679035632935 21 -0.097091952357061473 22 -0.089593138196590871
		 23 -0.080969039091314698 24 -0.071418294137089333 25 -0.061138894129769028 26 -0.050327660142301056
		 27 -0.039179797151390972 28 -0.027888523195429116 29 -0.016644774649506644 30 -0.0056369882259588167
		 31 0.0049490397424420962 32 0.014930216291077159 33 0.024126136792446573 34 0.032358991792335701
		 35 0.039453395780256528 36 0.045236138123627256 37 0.049535856521572441 38 0.052182633407085177
		 39 0.053007515735657762 40 0.052588354893103428 41 0.051640404404764695 42 0.050201501014313976
		 43 0.048309864305721817 44 0.046004088810207122 45 0.043323131297697159 46 0.04030629326956614
		 47 0.036993198671197547 48 0.033423766844776992 49 0.029638180743532416 50 0.025676850427885256
		 51 0.02158037186279671 52 0.017389481032665799 53 0.013145003386398457 54 0.0088877986198525605
		 55 0.0046587007972362926 56 0.00049845380506496895 57 -0.0035523578749661056 58 -0.0074533830966885262
		 59 -0.011164582715025517 60 -0.014646313026048574 61 -0.018075007522909914 62 -0.021642679263266018
		 63 -0.025337314684648504 64 -0.029150866595287539 65 -0.033077803091643432 66 -0.037113786933318238
		 67 -0.041254483701607114 68 -0.045494498054821975 69 -0.049826438285805352 70 -0.054240110103677353
		 71 -0.058721841056978974 72 -0.063253937265713489 73 -0.067814274141895226 74 -0.072376022571546628
		 75 -0.076907511649338289 76 -0.081372228548588055 77 -0.085728955536552576 78 -0.089932043572190423
		 79 -0.093931821413505645 80 -0.097675138777319148 81 -0.10110604188498939 82 -0.10416657973503618
		 83 -0.10679773968951167 84 -0.10894051145002948 85 -0.11053707920936517 86 -0.11153214264966493
		 87 -0.11187436843644605;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_pelvis_jnt_002_cnt_translateY";
	rename -uid "82DD9344-45C3-3794-F029-3BB7B7244CCF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  17 -3.5360146375232091 18 -3.5037516037048988
		 19 -3.4110155799356505 20 -3.263954803709197 21 -3.0687866176901366 22 -2.8317714996959751
		 23 -2.5591894856216157 24 -2.2573189639875579 25 -1.9324178324878005 26 -1.590707017368473
		 27 -1.2383563649011506 28 -0.8814729201714897 29 -0.52609161170325081 30 -0.1781683611465894
		 31 0.15642436436047547 32 0.4718995430715438 33 0.76255512354777011 34 1.0227710800272547
		 35 1.2470040003678946 36 1.4297792138234797 37 1.5656804698920297 38 1.6493371817933138
		 39 1.6754092484232928 40 1.6621608261484653 41 1.6321989425709376 42 1.586719504146032
		 43 1.526930517768605 44 1.4540518412946994 45 1.369314781865441 46 1.2739615425526836
		 47 1.1692445179198074 48 1.0564254391418473 49 0.93677436935084302 50 0.81156854985924554
		 51 0.68209109786650401 52 0.54962955616859688 53 0.41547429526630708 54 0.28091676810620925
		 55 0.14724761749300797 56 0.015754635984833953 57 -0.11227942216997633 58 -0.23557917776297188
		 59 -0.35287911300842723 60 -0.46292620883346558 61 -0.57129700098118974 62 -0.68406044869581706
		 63 -0.80083683914953951 64 -0.92137182464664136 65 -1.045490558238626 66 -1.1730559527112945
		 67 -1.3039310100351145 68 -1.4379451995749548 69 -1.5748648915445784 70 -1.7143678748585813
		 71 -1.8560220041623694 72 -1.9992680287589479 73 -2.1434066565033163 74 -2.2875898992368349
		 75 -2.4308167342377516 76 -2.5719331001109538 77 -2.7096362274354817 78 -2.8424832863688607
		 79 -2.968904317311015 80 -3.087219398560805 81 -3.1956599982996323 82 -3.2923944584596683
		 83 -3.3755575658162087 84 -3.4432841810894814 85 -3.4937469192995536 86 -3.5251979025703974
		 87 -3.5360146375232091;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_pelvis_jnt_002_cnt_translateZ";
	rename -uid "8DA5669D-4DC4-D892-8091-CDB9098BDEC6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  17 -2.4618148678737684 18 -2.4470055699600075
		 19 -2.4040960320761031 20 -2.3353564012961989 21 -2.2430502223804862 22 -2.12943835638126
		 23 -1.9967825170874192 24 -1.8473482383414725 25 -1.6834071512622388 26 -1.5072385093724183
		 27 -1.3211299516531159 28 -1.1273775386528675 29 -0.92828513493909848 30 -0.72616324232900142
		 31 -0.52332741238713154 32 -0.32209638353677544 33 -0.1247900977200542 34 0.066272246201350193
		 35 0.2487739502161456 36 0.42040224647580027 37 0.57884927830529787 38 0.72181254011942242
		 39 0.84699471770034174 40 0.96267543648046605 41 1.0782788424269425 42 1.1932283259690604
		 43 1.3069479826825467 44 1.4188627569954033 45 1.5283985676321115 46 1.6349824095682468
		 47 1.7380424276215385 48 1.8370079572711342 49 1.9313095288735047 50 2.02037883213031
		 51 2.1036486384597999 52 2.1805526798283146 53 2.2505254836116189 54 2.3130021641768703
		 55 2.3674181731048778 56 2.4132090113092488 57 2.4498099077547488 58 2.4766554710325459
		 59 2.4931793217166263 60 2.4988137152051317 61 2.4789911624544887 62 2.4215191421002635
		 63 2.3293778469368589 64 2.2055360371854245 65 2.052958687328744 66 1.8746139178791581
		 67 1.6734787619145599 68 1.4525434699139934 69 1.2148141942484869 70 0.96331401693962915
		 71 0.70108239106071735 72 0.4311731572994929 73 0.15665137246512656 74 -0.1194107542312772
		 75 -0.39393847880872768 76 -0.66385933011789544 77 -0.92610773489227871 78 -1.1776290962749383
		 79 -1.4153829874483597 80 -1.6363451644588842 81 -1.8375081613625646 82 -2.0158803060834294
		 83 -2.1684830865327918 84 -2.2923469033271195 85 -2.3845053677375336 86 -2.4419884413760991
		 87 -2.4618148678737684;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_pelvis_jnt_002_cnt_rotateX";
	rename -uid "145C1132-4F95-71FD-2CE6-7BA38A50C4CA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  17 -6.4901267153736599 18 -6.4509178792785393
		 19 -6.3373284543995725 20 -6.1554141044888606 21 -5.9112305278882644 22 -5.6108334370859394
		 23 -5.2602785402446006 24 -4.8656215256686277 25 -4.4329180498439262 26 -3.9682237293855964
		 27 -3.4775941369620318 28 -2.9670848010295132 29 -2.4427512090092383 30 -1.9106488133704489
		 31 -1.3768330399506001 32 -0.8473592977484723 33 -0.32828298937029365 34 0.17434047870645092
		 35 0.65445568686088484 36 1.1060071947907983 37 1.5229395363441551 38 1.8991972172087186
		 39 2.2287247157643333 40 2.5333062697015518 41 2.8377560575834324 42 3.1405634484599267
		 43 3.4402178076636747 44 3.7352084960569112 45 4.0240248693757037 46 4.3051562776988979
		 47 4.5770920650671512 48 4.8383215692748909 49 5.0873341218547816 50 5.3226190482706759
		 51 5.5426656683308009 52 5.7459632968279992 53 5.9310012444088169 54 6.0962688186672604
		 55 6.2402553254528232 56 6.3614500703755912 57 6.458342360483992 58 6.5294215060827039
		 59 6.573176822650038 60 6.5880976328048595 61 6.5356062062014919 62 6.3834475751062518
		 63 6.1395952819841186 64 5.8120229292333905 65 5.4087041393068978 66 4.9376125184941611
		 67 4.4067216266950817 68 3.8240049547476453 69 3.1974359101784571 70 2.5349878116193501
		 71 1.8446338915746019 72 1.1343473067332737 73 0.4121011546058243 74 -0.31413150506936549
		 75 -1.0363776259490651 76 -1.7466641496872473 77 -2.4370179815159734 78 -3.0994659687551565
		 79 -3.7260348840519386 80 -4.3087514149057231 81 -4.8396421607015929 82 -5.310733638059518
		 83 -5.7140522948163968 84 -6.0416245323986972 85 -6.2854767357171424 86 -6.4376353090201626
		 87 -6.4901267153736599;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_pelvis_jnt_002_cnt_rotateY";
	rename -uid "C859172D-493B-204C-59FB-84AAF8568E4E";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  17 0.20489847672761008 18 0.20366588387806758
		 19 0.20009448015391285 20 0.19437321359654647 21 0.18669048327611465 22 0.17723446512106744
		 23 0.16619340597019355 24 0.15375587030063806 25 0.14011092957372165 26 0.12544828904378125
		 27 0.10995835120115521 28 0.093832218770969061 29 0.077261643362244184 30 0.060438928451604049
		 31 0.043556797385276985 32 0.026808238484635059 33 0.010386340137457956 34 -0.0055158710553950491
		 35 -0.02070557643258766 36 -0.034990284114853144 37 -0.048177910573005245 38 -0.06007681720596731
		 39 -0.070495797060860496 40 -0.080123967462789908 41 -0.089745705334940695 42 -0.099313019378769546
		 43 -0.10877797692637559 44 -0.11809271588938769 45 -0.12720945518950746 46 -0.13608050323600981
		 47 -0.14465826404493454 48 -0.15289524063341306 49 -0.16074403537052653 50 -0.1681573470231908
		 51 -0.17508796430177634 52 -0.18148875578540991 53 -0.187312656191159 54 -0.19251264904451071
		 55 -0.19704174591074736 56 -0.20085296245799258 57 -0.20389929174292615 58 -0.2061336752394991
		 59 -0.20750897226957779 60 -0.20797792864241155 61 -0.20632807722350369 62 -0.2015446229202745
		 63 -0.19387561277478504 64 -0.18356814323171905 65 -0.17086899596846913 66 -0.15602521426458066
		 67 -0.13928458247877404 68 -0.12089598398311949 69 -0.1011096243651303 70 -0.080177116874709839
		 71 -0.058351435970024422 72 -0.035886752394278479 73 -0.013038169472417356 74 0.0099386147747533658
		 75 0.032787691570081441 76 0.055253341113993916 77 0.077080417140739696 78 0.098014686035174201
		 79 0.11780309237631556 80 0.13619392630365051 81 0.15293687300970896 82 0.16778293091951563
		 83 0.18048419269653806 84 0.19079349209407653 85 0.19846392984039715 86 0.20324830321105727
		 87 0.20489847672761008;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_pelvis_jnt_002_cnt_rotateZ";
	rename -uid "AD6582B4-4A28-13F1-2A66-B481E3C1D2EE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  17 -0.011617280006507906 18 -0.011477485244382699
		 19 -0.011077256570686159 20 -0.010451038956358618 21 -0.0096390291821990947 22 -0.008685014211812803
		 23 -0.0076344087497776433 24 -0.0065324902211578084 25 -0.0054228303762257537 26 -0.0043459235892806579
		 27 -0.0033380126182387957 28 -0.0024301130843781306 29 -0.0016472382045958258 30 -0.0010078253670178317
		 31 -0.00052336600675921964 32 -0.0001982399471208214 33 -2.9754967601215219e-05 34 -8.39189510296379e-06
		 35 -0.00011825504290329146 36 -0.00033772739677943018 37 -0.00064032961850943771
		 38 -0.00099578174519859754 39 -0.0013712664388041602 40 -0.001771610908413471 41 -0.0022229259024350578
		 42 -0.002722513194187157 43 -0.0032666674299485195 44 -0.0038506968886076339 45 -0.0044689569083859277
		 46 -0.0051148959377253292 47 -0.0057811141611949961 48 -0.0064594346471293536 49 -0.0071409869619275135
		 50 -0.007816303196997592 51 -0.0084754263582383534 52 -0.0091080310750907088 53 -0.0097035565964064142
		 54 -0.010251352053732706 55 -0.010740833988840921 56 -0.011161656161097931 57 -0.011503891671144781
		 58 -0.011758227459559978 59 -0.01191617126202699 60 -0.011970271124832958 61 -0.011780488669419824
		 62 -0.011238902031387008 63 -0.010397443145174651 64 -0.0093184783946466242 65 -0.0080709910358735815
		 66 -0.0067271068561070482 67 -0.0053589586914546909 68 -0.0040358880079708649 69 -0.0028219840832874438
		 70 -0.0017739632012627674 71 -0.00093939156483958833 72 -0.00035525628877527678 73 -4.6888862721302453e-05
		 74 -2.7244937344555149e-05 75 -0.00029654328539730829 76 -0.00084226546170116561
		 77 -0.0016395161883726583 78 -0.0026517429928073731 79 -0.0038318122943737918 80 -0.0051234381289626584
		 81 -0.0064629591544419053 82 -0.0077814595982302866 83 -0.0090072304517723797 84 -0.010068568494338029
		 85 -0.010896912586577849 86 -0.011430318987195185 87 -0.011617280006507906;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_005_cnt_translateX";
	rename -uid "C8F8C830-411C-1A8A-7B18-9B85FC93912B";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 1.0923366533910155 18 1.1115032345599332
		 19 1.1589541628497955 20 1.2196160956414133 21 1.2783670422267903 22 1.3200122677913555
		 23 1.3293006935060703 24 1.2955706751311311 25 1.2678307893334591 26 1.2460607355814375
		 27 1.1848047872943823 28 1.0901321378868829 29 0.96811386232286623 30 0.82484143602320614
		 31 0.66643696973969213 32 0.49905349760325635 33 0.32886588696354124 34 0.16205466900717624
		 35 0.0047863107027978913 36 -0.13680584915312011 37 -0.25663560232004556 38 -0.34866876975416972
		 39 -0.40691583521214625 40 -0.43307405647567521 41 -0.43337917962963957 42 -0.40865221026211884
		 43 -0.35967741651188589 44 -0.29373957688554242 45 -0.2190621948513467 46 -0.13780644020246768
		 47 -0.052138260248796087 48 0.035770717157646459 49 0.1237427126737316 50 0.20959373558048355
		 51 0.29113391146589152 52 0.36616817391319501 53 0.43249729900276179 54 0.48791926625119686
		 55 0.53023093688113931 56 0.55723005005845039 57 0.56671754992325418 58 0.5463309435618271
		 59 0.49259971267596825 60 0.41667287007976483 61 0.32968997095289865 62 0.24276654670424591
		 63 0.16698872757874028 64 0.11341755850358481 65 0.093103511552840246 66 0.086846186188807906
		 67 0.070483580055480388 68 0.047647669021586125 69 0.022006826895903941 70 -0.0027379953180570737
		 71 -0.022883870472298895 72 -0.034774425561636235 73 -0.034847982992545212 74 -0.019663525365018586
		 75 0.014114721533388774 76 0.066010200185132817 77 0.12629174096903739 78 0.20272867781142168
		 79 0.30481871097396152 80 0.42463092713242645 81 0.55422336906261194 82 0.68564640667457866
		 83 0.81095005485727256 84 0.92219238787888003 85 1.0114454855334998 86 1.0707957205155481
		 87 1.0923366533910155 88 1.1115032345599332 89 1.1589541628497955 90 1.2196160956414133
		 91 1.2783670422267903 92 1.3200122677913555 93 1.3293006935060703 94 1.3425929898579625
		 95 1.3963129950502946 96 1.464833028173075 97 1.5221406940601696 98 1.5524041767213816
		 99 1.5397006909020376 100 1.4726654311850211 101 1.3998634136570729 102 1.3311824263851975
		 103 1.2312573445832697 104 1.1062433453023175 105 0.96234313646914416 106 0.80578571740340976
		 107 0.64280093147715434 108 0.47959401059350171 109 0.32232448891124932 110 0.17709351174266885
		 111 0.049942690442833282 112 -0.053133734730749893 113 -0.12616355464450635 114 -0.16972236570313726
		 115 -0.18527525009989176 116 -0.1758439434288448 117 -0.14899692830280742 118 -0.10690034163653195
		 119 -0.05171706930872233 120 0.01439062500782029 121 0.08925890236511691 122 0.17072059525804661
		 123 0.25660389933179317 124 0.34473150118697049 125 0.43292012610891106 126 0.51898048464803992
		 127 0.60071759449948559 128 0.67593145406107169 129 0.74241804642866782 130 0.79797065741536244
		 131 0.84038149847151544 132 0.86744363514412726 133 0.87695323393552371 134 0.85651911118952739
		 135 0.80266209813075307 136 0.73894496167966395 137 0.68593889529981311 138 0.65035187556421192
		 139 0.63894274314168342 140 0.65851343293172704 141 0.71587261736866026 142 0.77506397696470231
		 143 0.7980387621028342 144 0.78848408218604504 145 0.75007911592535947 146 0.68643238178862021
		 147 0.60105024949007202 148 0.51550698793892025 149 0.45315678565413009 150 0.42007691631241073
		 151 0.42231349240753957 152 0.46211819905803964 153 0.53241865234619468 154 0.62527872597433998
		 155 0.73278849033005145 156 0.84706433335222187;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_005_cnt_translateY";
	rename -uid "99CDDADD-4FAD-6564-A864-7AA021CCEA02";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -0.014066926832704496 18 -0.0042871084143456528
		 19 0.020441171284460324 20 0.053155952477574431 21 0.087073287669227284 22 0.11574870472753673
		 23 0.1330743903940288 24 0.13434056720613086 25 0.13076343938271506 26 0.13131484302878604
		 27 0.13263316398144553 28 0.13375417330493278 29 0.13335855726147017 30 0.13005752048224473
		 31 0.12263325931104241 32 0.11023426375799783 33 0.092525077086798468 34 0.069790000621189119
		 35 0.042990278498152401 36 0.013774491851223258 37 -0.015557834641597545 38 -0.042139141688487536
		 39 -0.062662805422009527 40 -0.075863710309461041 41 -0.081760194441923062 42 -0.078850293264531857
		 43 -0.065979870335542756 44 -0.04644342944640556 45 -0.024483917683014056 46 -0.00079030353780851215
		 47 0.023964311226048807 48 0.049126170094435651 49 0.07406296933169898 50 0.098164588189369795
		 51 0.12084243497773173 52 0.14152740241348027 53 0.15966642654978358 54 0.17471764509790688
		 55 0.18614415300934439 56 0.19340635579834498 57 0.19595292424389044 58 0.19047744557946089
		 59 0.17598436300780662 60 0.15535136666830596 61 0.13149326615116408 62 0.10741564691181438
		 63 0.086232612592525015 64 0.071148750094687685 65 0.065405458370523029 66 0.067178665186260034
		 67 0.070742291781441313 68 0.072192480151969329 69 0.067268183011537808 70 0.052699745411723953
		 71 0.027170616316626983 72 -0.0081115370104249962 73 -0.049237743127477529 74 -0.0898153191416311
		 75 -0.12157737179021488 76 -0.13652374120863486 77 -0.12994259700713084 78 -0.11099977626024771
		 79 -0.09166267830234176 80 -0.073063907419875562 81 -0.056259630326866983 82 -0.042053869159477841
		 83 -0.030897669313304732 84 -0.022863101145290443 85 -0.017691900639690061 86 -0.014918460638099873
		 87 -0.014066926832704496 88 -0.0042871084143456528 89 0.020441171284460324 90 0.053155952477574431
		 91 0.087073287669227284 92 0.11574870472752252 93 0.13307439039400748 94 0.14688465523570926
		 95 0.16496951375830093 96 0.18935166011810622 97 0.22193026428060136 98 0.25613023985891914
		 99 0.28513819281871022 100 0.30321832650505698 101 0.32007924002200383 102 0.33582063560298892
		 103 0.33929283114004605 104 0.33054322482091614 105 0.3102189252154961 106 0.27961637998115307
		 107 0.24068420527065015 108 0.19597909660314095 109 0.14857498603689123 110 0.10192584714913622
		 111 0.059682665589628314 112 0.025465036622223991 113 0.0025876045588297814 114 -0.010071894959491345
		 115 -0.014606545224658873 116 -0.011855810468581751 117 -0.0040409909667786792 118 0.008166979902142657
		 119 0.024085446921279896 120 0.043028993830098727 121 0.06431703365226582 122 0.087280032110385264
		 123 0.11126436404565254 124 0.13563579993375185 125 0.1597816175746587 126 0.18311133281586933
		 127 0.20505604270154265 128 0.22506637467515844 129 0.24260903633126674 130 0.25716196167191896
		 131 0.26820805182087071 132 0.27522751066739914 133 0.27768877896510347 134 0.27239659217297785
		 135 0.25838659831333288 136 0.24545232439501774 137 0.24022161220419136 138 0.23955754048335542
		 139 0.24006782400348659 140 0.23938953743184044 141 0.23706961520721093 142 0.22339072176284613
		 143 0.19057829206688837 144 0.14459708275049321 145 0.093289433722466786 146 0.04548685759720339
		 147 0.009735433845659891 148 -0.018885617507140573 149 -0.048007886576817427 150 -0.074400706935215055
		 151 -0.094806575175859109 152 -0.10695604792444158 153 -0.11114509750347423 154 -0.10768268795786895
		 155 -0.096612524433584923 156 -0.077770548879861678;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_005_cnt_translateZ";
	rename -uid "31581386-43F9-C248-E461-AE903C5C3E0E";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 3.0502263938414025 18 3.0173338696339123
		 19 2.930971250783859 20 2.8096060774452418 21 2.671722821308304 22 2.5358384128001461
		 23 2.4205023517356814 24 2.3437329283662152 25 2.3164006133115507 26 2.2676836427793745
		 27 2.1289053902593196 28 1.911091779521616 29 1.6252527048146224 30 1.2824126896946382
		 31 0.89363329523371426 32 0.47002547267763006 33 0.022751661469012419 34 -0.43698126562928152
		 35 -0.89793611479432656 36 -1.3488637629509057 37 -1.7785226853205565 38 -2.1756964289413752
		 39 -2.5292061177608076 40 -2.8269985287860493 41 -3.0565149067224766 42 -3.205863636925848
		 43 -3.263149963447411 44 -3.271043106020052 45 -3.2799834779785328 46 -3.2897126833920503
		 47 -3.2999716447769174 48 -3.3105004712304265 49 -3.3210383877976604 50 -3.3313237235572686
		 51 -3.3410939556121555 52 -3.3500858061623973 53 -3.3580353901195767 54 -3.3646784112980139
		 55 -3.3697504060897039 56 -3.3729870346965889 57 -3.3741244214569139 58 -3.3716804426512077
		 59 -3.3652394495370235 60 -3.3561387442000594 61 -3.3457142432885352 62 -3.3352983665270592
		 63 -3.326219270326769 64 -3.3198014869628985 65 -3.317368029628831 66 -3.2137272859984627
		 67 -2.9272012688423139 68 -2.4943388419860675 69 -1.9517557871306721 70 -1.3362301259750211
		 71 -0.6847066352490927 72 -0.03422323832494456 73 0.57820514376607868 74 1.1156983512812815
		 75 1.5415717533361217 76 1.8197630796160151 77 1.9153762182225442 78 1.9412979665318328
		 79 2.0242533700267522 80 2.1501297548364242 81 2.3048151872910125 82 2.4742116711870121
		 83 2.6442450015715675 84 2.8008703096336056 85 2.9300716566708083 86 3.0178539936478899
		 87 3.0502263938414025 88 3.0173338696339123 89 2.930971250783859 90 2.8096060774452418
		 91 2.671722821308304 92 2.5358384128001603 93 2.4205023517356814 94 2.338095277445472
		 95 2.3009960461732426 96 2.2414522942339801 97 2.0884567428628742 98 1.8556625654865826
		 99 1.5567189363003955 100 1.2047452320533232 101 0.80571188957465623 102 0.37028180271146383
		 103 -0.085400534462696243 104 -0.55012666964007906 105 -1.0126658085806604 106 -1.461782693462343
		 107 -1.8862554900161967 108 -2.2748905153099841 109 -2.6165310414831282 110 -2.9000581318841405
		 111 -3.1143825009973476 112 -3.2484277345458601 113 -3.2911068594702257 114 -3.2858910535367727
		 115 -3.2840288168696929 116 -3.2851580758975309 117 -3.2883727051804144 118 -3.2934135904229649
		 119 -3.3000220869848889 120 -3.3079396433961321 121 -3.31690748601374 122 -3.326666366359909
		 123 -3.3369563712381538 124 -3.3475167945649442 125 -3.3580860689810859 126 -3.3684017547203133
		 127 -3.3782005829131645 128 -3.3872185504938983 129 -3.3951910641646172 130 -3.4018531314441756
		 131 -3.4069395977076944 132 -3.4101854292905394 133 -3.4113260441972777 134 -3.4088751250843075
		 135 -3.4024157734390474 136 -3.2918820548287826 137 -3.0009565153187125 138 -2.566557150359035
		 139 -2.0256699860692429 140 -1.4154443263833407 141 -0.77319708356889194 142 -0.1312191614294882
		 143 0.47845962173352063 144 1.0189264696924187 145 1.453452304665916 146 1.745483444259289
		 147 1.85853860881301 148 1.9038530473037092 149 2.0064945644956964 150 2.1506749756376902
		 151 2.3206089674372574 152 2.5009777103312878 153 2.6776012795088349 154 2.8364325979332659
		 155 2.9634516234999104 156 3.0446577444101734;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_005_cnt_rotateX";
	rename -uid "140109E3-465B-CC6A-C028-59801C1105F4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 4.991685740482569 18 4.9375627864797123
		 19 4.7954975361578818 20 4.5959451482766198 21 4.3693616374506199 22 4.1462046542701101
		 23 3.9569335169439959 24 3.8310926189692283 25 3.7863391077753747 26 3.7065028133282771
		 27 3.4791298246918352 28 3.1224211642213677 29 2.6545757516145532 30 2.0937922644724192
		 31 1.4582707235919878 32 0.7662137711711321 33 0.035827608198575327 34 -0.71467743857453458
		 35 -1.4670867394979923 36 -2.2031815787751783 37 -2.9047397500391421 38 -3.5535364943685179
		 39 -4.1313457222205914 40 -4.6184058780559072 41 -4.994044401006466 42 -5.2386697534176649
		 43 -5.3326923219844602 44 -5.3459009525319994 45 -5.3608532126992205 46 -5.3771141115520189
		 47 -5.3942484362665679 48 -5.4118207301760792 49 -5.429395293553287 50 -5.4465362029798925
		 51 -5.4628073446737506 52 -5.4777724571430637 53 -5.490995179011037 54 -5.5020390988078329
		 55 -5.510467804957381 56 -5.5158449360959709 57 -5.5177342342464089 58 -5.5136743905936987
		 59 -5.5029715843784306 60 -5.4878411049084139 61 -5.4704980402672634 62 -5.4531568209361163
		 63 -5.4380309651738381 64 -5.427333125148496 65 -5.4232755327745465 66 -5.2535378873221026
		 67 -4.7844537432324739 68 -4.0762203804723116 69 -3.1890380522760888 70 -2.1831067009331102
		 71 -1.1186221652535928 72 -0.055772227667516348 73 0.94526664082643286 74 1.8243307846602101
		 75 2.5212681606178249 76 2.9766914299761225 77 3.1330846254266453 78 3.1752197729030405
		 79 3.3107358070433488 80 3.5165890382118294 81 3.769736674187373 82 4.0471379130755327
		 83 4.3257544764549927 84 4.582550638264693 85 4.7944927958257022 86 4.9385486129667306
		 87 4.991685740482569 88 4.9375627864797123 89 4.7954975361578818 90 4.5959451482766198
		 91 4.3693616374506199 92 4.1462046542701154 93 3.9569335169439976 94 3.8217189049131095
		 95 3.7607317458835769 96 3.6629148760655816 97 3.4119506789768335 98 3.0304062659217195
		 99 2.5408469605727331 100 1.964921771187855 101 1.3123926159828039 102 0.60071501847130493
		 103 -0.14367491430777674 104 -0.90256784005534363 105 -1.6577513885834798 106 -2.3910103817608497
		 107 -3.0841271618645965 108 -3.718881974098903 109 -4.2770533759226863 110 -4.7404186730533384
		 111 -5.0907544096517361 112 -5.3098369679319832 113 -5.379443359688505 114 -5.3707281546325882
		 115 -5.3676157316200843 116 -5.3695031512085114 117 -5.3748751976447524 118 -5.3832967134442384
		 119 -5.3943326535637866 120 -5.4075479729001357 121 -5.4225075364106772 122 -5.4387760543811234
		 123 -5.4559180429823799 124 -5.4734978083522901 125 -5.4910794510060255 126 -5.5082268864234187
		 127 -5.5245038771822896 128 -5.5394740720045537 129 -5.5527010475577265 130 -5.5637483498077236
		 131 -5.5721795331506714 132 -5.5775581974612969 133 -5.5794480255825372 134 -5.5753870374968626
		 135 -5.5646811138768095 136 -5.3835065950937935 137 -4.907111269304739 138 -4.1963219335911699
		 139 -3.3119683261889965 140 -2.3148801726332131 141 -1.2658837672090431 142 -0.21729989705461247
		 143 0.7789828913619905 144 1.6628006616783475 145 2.3740007576027398 146 2.852437623322555
		 147 3.0379651245921537 148 3.1125447579504271 149 3.2810069718609118 150 3.5175018478339526
		 151 3.7961792897741913 152 4.0919474774978539 153 4.381588127156042 154 4.6420630438431898
		 155 4.85033615503737 156 4.9833732206483621;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_005_cnt_rotateY";
	rename -uid "E54B2EE8-4EE7-D2F9-9321-D2B89F73C1A0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0.15980335414272334 18 0.15814980135195009
		 19 0.15379309673472302 20 0.14765020955166608 21 0.14063689601726506 22 0.13364946230747796
		 23 0.12754860193874853 24 0.12315938645026357 25 0.12145536519091833 26 0.11868796857344448
		 27 0.11087926992330585 28 0.098839597097630613 29 0.083437379990916716 30 0.065559559000735693
		 31 0.046077355900556033 32 0.025817333237617836 33 0.0055377712756181008 34 -0.014089535332989707
		 35 -0.032491917282699317 36 -0.04920275073912924 37 -0.063862514410128662 38 -0.076213870763766248
		 39 -0.0860906997206905 40 -0.093500245180337449 41 -0.098557137133505668 42 -0.10130550765102742
		 43 -0.10173291099819851 44 -0.10091844810622082 45 -0.10002681106908491 46 -0.099093695436589524
		 47 -0.098151658003457098 48 -0.09722943482126932 49 -0.096351531237764756 50 -0.095538084242737797
		 51 -0.094804997383154982 52 -0.094164348465814604 53 -0.093625070200787461 54 -0.093193903865360722
		 55 -0.09287662599662494 56 -0.092679548050180693 57 -0.092611288897297778 58 -0.092758601228718041
		 59 -0.093158300746725528 60 -0.093751425211286377 61 -0.094471742437571504 62 -0.095235203524790327
		 63 -0.095936431580233378 64 -0.096452242046798004 65 -0.096652189931791352 66 -0.094602111600058728
		 67 -0.088612993893622691 68 -0.078678546647681452 69 -0.064721571213033374 70 -0.046855958453932867
		 71 -0.02557283327218941 72 -0.0018505930544384427 73 0.022810611520400852 74 0.046428535631851585
		 75 0.066650602052153188 76 0.080897563600591124 77 0.086568077396796742 78 0.089041386609184342
		 79 0.094729701306598968 80 0.10285075667488215 81 0.11263240666694425 82 0.12328756584730753
		 83 0.13399818091294147 84 0.14390817875493905 85 0.15212529874609951 86 0.1577317110122875
		 87 0.15980335414272334 88 0.15814980135195009 89 0.15379309673472302 90 0.14765020955166608
		 91 0.14063689601726506 92 0.13364946230745386 93 0.12754860193874842 94 0.12329524370989986
		 95 0.12181697138042161 96 0.11927431365627997 97 0.1117865446620763 98 0.10016653963809456
		 99 0.08529560746515065 100 0.068051183625861295 101 0.049378350328419862 102 0.03011937272332622
		 103 0.010895687171734439 104 -0.0076980369513839414 105 -0.025184678373453125 106 -0.041201770048308807
		 107 -0.05549271097796564 108 -0.06789198599434261 109 -0.078304336498220772 110 -0.086677865090602949
		 111 -0.092971091093157321 112 -0.097114000174545204 113 -0.098963151888769169 114 -0.099455606006542935
		 115 -0.099634124809694413 116 -0.099525701849233345 117 -0.099219912572274041 118 -0.09874890450375956
		 119 -0.098147132154301014 120 -0.097449587339855231 121 -0.096690301401122503 122 -0.095901119211278307
		 123 -0.095110745020452553 124 -0.094344060297601423 125 -0.093621713793323319 126 -0.092959984064794915
		 127 -0.092370914685161704 128 -0.09186272231279885 129 -0.091440477737190884 130 -0.091107059952366468
		 131 -0.090864383249414801 132 -0.090714897267654554 133 -0.090663359898762064 134 -0.090774739378376837
		 135 -0.091079710135787764 136 -0.089351031538813047 137 -0.083537230002501725 138 -0.073610326058831957
		 139 -0.059471036310818848 140 -0.041219631711184972 141 -0.019352781313275533 142 0.0048642985043709222
		 143 0.029672197858518524 144 0.053085620185942956 145 0.072734122382236727 146 0.086054208731161819
		 147 0.090550578775702564 148 0.091673709557721528 149 0.095961534049566255 150 0.10281405995344106
		 151 0.11161902101677809 152 0.1216797337291993 153 0.13215073041468053 154 0.1421126011166379
		 155 0.15059761986514672 156 0.15661695636350226;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_005_cnt_rotateZ";
	rename -uid "7913CF18-4F53-FF40-D06E-F5918825F2C0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -1.7821391193822094 18 -1.8138280029204055
		 19 -1.8922386249312186 20 -1.9923840969539901 21 -2.0892837727564935 22 -2.1579708080311084
		 23 -2.1734939228834431 24 -2.1184400892020103 25 -2.0730733264871111 26 -2.0375419185668306
		 27 -1.9375763726911954 28 -1.7830997237959458 29 -1.5840210238982633 30 -1.3502493131433921
		 31 -1.0917047910863122 32 -0.81832683510018478 33 -0.5400788241634733 34 -0.26694997984335866
		 35 -0.0089546319902342481 36 0.22387054681554738 37 0.42147071862369623 38 0.57377783762605139
		 39 0.67071580889780191 40 0.71484143521663113 41 0.71622069451378012 42 0.67604294445453217
		 43 0.59550716638014212 44 0.48683591932241438 45 0.36383417839702226 46 0.2300844571958327
		 47 0.089169295879765617 48 -0.05532873617127345 49 -0.1998270404325454 50 -0.34074299153878473
		 51 -0.47449394126815397 52 -0.59749722365106728 53 -0.70617016070583072 54 -0.79693006841610436
		 55 -0.86619426273613276 56 -0.91038006563840235 57 -0.92590481150526149 58 -0.89254406768489103
		 59 -0.80459306598994718 60 -0.68024867533856404 61 -0.53770774045538006 62 -0.39516702708203361
		 63 -0.270823191451668 64 -0.18287278587903225 65 -0.14951231229024356 66 -0.13957514615956237
		 67 -0.11359800901976384 68 -0.077296773304856486 69 -0.036361328401409265 70 0.0035159917747254609
		 71 0.036607538258070976 72 0.057115651752447788 73 0.059143257064028711 74 0.036679001775239695
		 75 -0.016393989297203246 76 -0.10002742692136204 77 -0.19877181356564222 78 -0.32465221059063221
		 79 -0.49235556911953654 80 -0.68889237895888666 81 -0.90127281351454802 82 -1.116514262223395
		 83 -1.3216463306543842 84 -1.5037131078066075 85 -1.6497723700736524 86 -1.7468913875463488
		 87 -1.7821391193822094 88 -1.8138280029204055 89 -1.8922386249312186 90 -1.9923840969539901
		 91 -2.0892837727564935 92 -2.1579708080311217 93 -2.1734939228834622 94 -2.1954588962285264
		 95 -2.283473642892881 96 -2.3956729313895599 97 -2.4895323789567052 98 -2.5391043491921104
		 99 -2.5184309903904158 100 -2.409086299424219 101 -2.2903207846580047 102 -2.1782187770838859
		 103 -2.0151419698756445 104 -1.8110874289832783 105 -1.5760770994570246 106 -1.3201544428451111
		 107 -1.0533798399790224 108 -0.78582538567920279 109 -0.52756963219881381 110 -0.28869270215730847
		 111 -0.079271989168423665 112 0.090621604223932783 113 0.21092727552309387 114 0.28260835444320132
		 115 0.30820872176445258 116 0.29268420129366246 117 0.24849900847726042 118 0.17923567690451192
		 119 0.088476726672720704 120 -0.020195322134117486 121 -0.14319793867063677 122 -0.27694857356283814
		 123 -0.41786465416637991 124 -0.56236358264314845 125 -0.70686273647611753 126 -0.8477794709278047
		 127 -0.98153112288883959 128 -1.1045350155608802 129 -1.2132084634754967 130 -1.3039687774624744
		 131 -1.3732332693539537 132 -1.4174192564395798 133 -1.4329440659751489 134 -1.3995831846682323
		 135 -1.311631808489629 136 -1.2077462635009124 137 -1.1217323165957789 138 -1.0644774920032596
		 139 -1.046845841927784 140 -1.07970627880562 141 -1.1739652428829643 142 -1.2707679812506205
		 143 -1.3078952512454962 144 -1.2913406599674309 145 -1.2272036347792368 146 -1.1216548826578343
		 147 -0.98086991865077788 148 -0.83998547234597565 149 -0.73679135837323906 150 -0.68138736081507623
		 151 -0.68387624443822115 152 -0.74813973272883882 153 -0.86267755559635773 154 -1.0145386802852356
		 155 -1.1907857490834532 156 -1.3784896861678153;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_004_cnt_translateX";
	rename -uid "E1259594-442E-370C-B353-559E84F6EA97";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 2.0001123724088643 18 2.0361408154939795
		 19 2.1251350947803473 20 2.2384737311502363 21 2.3473735095221002 22 2.4227848045040048
		 23 2.4354250641404462 24 2.3641226003508677 25 2.2940763796812007 26 2.2310013036159546
		 27 2.1009587179543416 28 1.9147380509007235 29 1.6832139433489317 30 1.4173644753098813
		 31 1.1282551584089617 32 0.82698723971373056 33 0.52461687268095147 34 0.2320577329093112
		 35 -0.040016405254931442 36 -0.28125083177096144 37 -0.48165288638034554 38 -0.63158333165361569
		 39 -0.72168513022756997 40 -0.75621545336380791 41 -0.74735086584988153 42 -0.69762016781379543
		 43 -0.60929545486635561 44 -0.49335384956589223 45 -0.36192928304382121 46 -0.2187903296146203
		 47 -0.067726961647849748 48 0.087445840893451532 49 0.24288647611942338 50 0.3947269382459524
		 51 0.53907468525542868 52 0.6720159723307404 53 0.78962048514901539 54 0.88794714436684785
		 55 0.96305100934429788 56 1.0109912853916114 57 1.0278405345314923 58 0.99163703729252006
		 59 0.89625345141752177 60 0.76155578348246422 61 0.60737293819735783 62 0.45343631268912077
		 63 0.31935558288276411 64 0.22463462694946656 65 0.18873151202691929 66 0.17366745227778324
		 67 0.13424140890450076 68 0.079451057259717572 69 0.018561775461648722 70 -0.039163179245861102
		 71 -0.084774944515658035 72 -0.10995974841210909 73 -0.10729676926484899 74 -0.070368517106544459
		 75 0.0063506051653803297 76 0.12121128391109437 77 0.25620747323026194 78 0.41727170069424346
		 79 0.61481803277257541 80 0.83636520751369403 81 1.0693566949249487 82 1.3011759092785269
		 83 1.5191723337129304 84 1.7106899988639839 85 1.8630899089999104 86 1.9637602342826597
		 87 2.0001123724088643 88 2.0361408154939795 89 2.1251350947803473 90 2.2384737311502363
		 91 2.3473735095221002 92 2.4227848045040048 93 2.4354250641404747 94 2.4479294059820518
		 95 2.5232117705800192 96 2.6214895324387157 97 2.7035891247818427 98 2.741158873850992
		 99 2.7055168102419032 100 2.5760698568765861 101 2.43980362207148 102 2.3143851988606627
		 103 2.136219436016404 104 1.916198699955487 105 1.6652609934785687 106 1.3942881659907869
		 107 1.114000531243164 108 0.83486522363085669 109 0.56703565933892719 110 0.32033744634705386
		 111 0.10431213247224491 112 -0.07167556648568052 113 -0.19826885508541636 114 -0.2750298194232812
		 115 -0.30242788869466608 116 -0.28581425168601982 117 -0.23851171023682127 118 -0.1643096833728066
		 119 -0.066983874062458426 120 0.04969216625229933 121 0.18193746075272088 122 0.3259561183613755
		 123 0.47793193978725412 124 0.63402505019467981 125 0.79037043323680223 126 0.94307820264097586
		 127 1.08823542758293 128 1.2219093270379631 129 1.3401516663979862 130 1.4390042271267305
		 131 1.5145052772437282 132 1.5626970470310937 133 1.5796343104887285 134 1.5432414578176861
		 135 1.4473546537578272 136 1.3302929290566397 137 1.2262786553817762 138 1.1495360532852317
		 139 1.1146710478133457 140 1.1363729147549293 141 1.2290577148502848 142 1.3302140565489253
		 143 1.3728931939978395 144 1.3642711986695133 145 1.3111459512286103 146 1.219825063083789
		 147 1.0961608745942613 148 0.97034806845059052 149 0.87701559401352824 150 0.82831757246989923
		 151 0.83626185643777262 152 0.9061222557028259 153 1.0267524842091404 154 1.1855436703689008
		 155 1.3699796340105479 156 1.5676228557757099;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_004_cnt_translateY";
	rename -uid "34285AE0-4697-4901-D67C-B78272D3D082";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -0.22814777779467477 18 -0.20896845026349808
		 19 -0.15953294923630068 20 -0.092134363027270183 21 -0.018780677478055452 22 0.049105955245686062
		 23 0.10064140815844524 24 0.12745401797599953 25 0.15191010583000519 26 0.18931840467231353
		 27 0.2278259822127211 28 0.26206153343292016 29 0.2864377300753489 30 0.29585450877319985
		 31 0.28628133858066107 32 0.25521539304585872 33 0.20201166446170404 34 0.12808154309606934
		 35 0.036958029134055437 36 -0.06577190101554109 37 -0.1726647844174316 38 -0.27474308627381916
		 39 -0.36193678125565043 40 -0.42854019593520576 41 -0.47172231574262824 42 -0.48566530922705198
		 43 -0.46568439519705862 44 -0.42468116941755341 45 -0.37884737641419264 46 -0.32970436754769139
		 47 -0.27871399708272548 48 -0.22726538231362525 49 -0.17666678441607786 50 -0.12814266619355408
		 51 -0.082835985569744253 52 -0.041815780004903047 53 -0.00609008792670096 54 0.023375760375635934
		 55 0.04563546746078373 56 0.059732715170618178 57 0.064666763637625024 58 0.054051885945860079
		 59 0.025848136190369075 60 -0.014567696901728766 61 -0.061677555786523897 62 -0.10961889328036989
		 63 -0.1521181119365238 64 -0.18255954660395446 65 -0.19418880765966051 66 -0.17516388342041367
		 67 -0.12660175350544733 68 -0.065467204671605828 69 -0.010373389586220583 70 0.022495344140711637
		 71 0.022428580637068762 72 -0.013830600872848819 73 -0.081209895927472076 74 -0.16655358071665916
		 75 -0.24997528858705564 76 -0.30972539949335243 77 -0.32907064152391996 78 -0.31958794697630211
		 79 -0.30500137946944506 80 -0.28840995899101785 81 -0.27219432622764828 82 -0.25795567099339678
		 83 -0.24654019344491473 84 -0.23814903868697002 85 -0.23253342297811486 86 -0.22927451926673825
		 87 -0.22814777779467477 88 -0.20896845026349808 89 -0.15953294923630068 90 -0.092134363027270183
		 91 -0.018780677478055452 92 0.049105955245728694 93 0.10064140815848077 94 0.15162133387636345
		 95 0.21771604740649053 96 0.30066037770809828 97 0.39893106320984373 98 0.49678338217181306
		 99 0.57867257863645705 100 0.63158063044343038 101 0.67185548413981167 102 0.69871436538228693
		 103 0.69096111491427337 104 0.64907899148924386 105 0.57539858612762984 106 0.47418332946787345
		 107 0.3515811185411728 108 0.21544455230278459 109 0.075024285818567904 110 -0.059458844054098847
		 111 -0.17735745600207409 112 -0.26825334149032898 113 -0.32272495123095268 114 -0.34891645892883361
		 115 -0.3583210759566029 116 -0.35261477631257776 117 -0.33642726520064059 118 -0.3112116576141446
		 119 -0.27846538206028981 120 -0.23969648826923162 121 -0.19639506623901326 122 -0.15000974713111503
		 123 -0.1019292887687584 124 -0.05346927303506277 125 -0.005863959178235234 126 0.039736653789788079
		 127 0.082264500093167214 128 0.12072580377476783 129 0.15418807580784488 130 0.18176172163789062
		 131 0.20257624553752862 132 0.21575105781673187 133 0.22036091390511814 134 0.21044262537469649
		 135 0.18407425861829552 136 0.17598457057446737 137 0.20964772274685473 138 0.26939206850492425
		 139 0.33830456192227842 140 0.40205570618304165 141 0.45162245708939963 142 0.46186958933435562
		 143 0.41414834094772246 144 0.3199995922084149 145 0.1977739553152098 146 0.070592665430197599
		 147 -0.036744808195233958 148 -0.12673494023632159 149 -0.21369774799446262 150 -0.2911989298721025
		 151 -0.3522910841925011 152 -0.39206930203734913 153 -0.41126285313315236 154 -0.41099229529318393
		 155 -0.39216931182158277 156 -0.35552689205087518;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_004_cnt_translateZ";
	rename -uid "97081022-47E7-0407-10BD-7899918B6D02";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 5.5061799042934556 18 5.4686210116931573
		 19 5.3658236716352459 20 5.2125634975433215 21 5.0236213164245171 22 4.8138458759529694
		 23 4.598196997137963 24 4.3908401849209078 25 4.1939199069748234 26 3.9339711692776582
		 27 3.5502867954167385 28 3.058907945981769 29 2.4759922746647476 30 1.8179047428657142
		 31 1.1012570754238382 32 0.34289604433446996 33 -0.44015297356553518 34 -1.2307757996581508
		 35 -2.0118777606765654 36 -2.7664936684637773 37 -3.4778822422790263 38 -4.1295889331826992
		 39 -4.7054638491380096 40 -5.1878192262511114 41 -5.5580350954452129 42 -5.7987797512307182
		 43 -5.8925895420041847 44 -5.9082480521893714 45 -5.9259786133528962 46 -5.9452667320687276
		 47 -5.9655970114630712 48 -5.9864530844103427 49 -6.0073176425286547 50 -6.0276725405338674
		 51 -6.0469989530527606 52 -6.0647775608910095 53 -6.0804887460308557 54 -6.0936127793149737
		 55 -6.1036299918740013 56 -6.1100209308771252 57 -6.1122665121245223 58 -6.1074411116205916
		 59 -6.0947209646419314 60 -6.0767408814446338 61 -6.0561351305500786 62 -6.0355358341171481
		 63 -6.0175719231800961 64 -6.0048691457422301 65 -6.0000516190085982 66 -5.8310942346308128
		 67 -5.3612846970544492 68 -4.6458379859150121 69 -3.7402666575331285 70 -2.7010446587121173
		 71 -1.5857628992951636 72 -0.45281003920059248 73 0.63926004479011667 74 1.6324371702383367
		 75 2.4697833450660376 76 3.0963374683135232 77 3.4598030584901132 78 3.6910765874016747
		 79 3.9478517557462673 80 4.2182560323281777 81 4.4904461613706896 82 4.7526380162250916
		 83 4.9931234889283918 84 5.2002726039746143 85 5.3625199212957622 86 5.4683352679616597
		 87 5.5061799042934556 88 5.4686210116931573 89 5.3658236716352459 90 5.2125634975433215
		 91 5.0236213164245171 92 4.813845875952957 93 4.5981969971379613 94 4.3812998648295185
		 95 4.1676671346071164 96 3.88887545098072 97 3.4800687777435435 98 2.9616784780045187
		 99 2.3544752919609913 100 1.6786796899554233 101 0.9418799509369542 102 0.1600444846789415
		 103 -0.64061542465159249 104 -1.4427449445478118 105 -2.2290224023775771 106 -2.9822766571357988
		 107 -3.6855884933934284 108 -4.3223598939731112 109 -4.8763378554570309 110 -5.331583703734438
		 111 -5.6723845568169651 112 -5.8831105924463287 113 -5.9480300785004445 114 -5.9376912345310195
		 115 -5.9339993699283156 116 -5.9362381459984022 117 -5.9426106650402986 118 -5.9526018286901561
		 119 -5.9656969521791225 120 -5.981381317217128 121 -5.9991398198332035 122 -6.0184567257284236
		 123 -6.0388155339612837 124 -6.0596989403580004 125 -6.0805888849178551 126 -6.1009666626805004
		 127 -6.1203130750604018 128 -6.1381085985578707 129 -6.1538335500496313 130 -6.1669682325700155
		 131 -6.1769930526189327 132 -6.1833886095890911 133 -6.1856357698802018 134 -6.1808069482937036
		 135 -6.1680772816790164 136 -5.9852916337165922 137 -5.5061666344286948 138 -4.7866768219562132
		 139 -3.8829822649453956 140 -2.8520970600995064 141 -1.75205869546493 142 -0.63215444375330776
		 143 0.45800743842626268 144 1.4596824595149585 145 2.315145759605393 146 2.9679631588446771
		 147 3.3627589737664962 148 3.6277431109015073 149 3.9180527581726974 150 4.2191650256729449
		 151 4.5166482051912418 152 4.7968903971287027 153 5.0481539122422383 154 5.2588824869240529
		 155 5.4175266919452163 156 5.5125307383543678;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_004_cnt_rotateX";
	rename -uid "10492EAE-4B51-DA53-AC62-E5BBF21A53F6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 9.914353778276686 18 9.85003708465009
		 19 9.6727227013240054 20 9.4058752246502451 21 9.0729740426217447 22 8.6974957821290229
		 23 8.3028765554085435 24 7.9105531972130452 25 7.5172110333693229 26 7.0036586107889285
		 27 6.2786822296287346 28 5.3706811191731534 29 4.30807946572265 30 3.1193620314475723
		 31 1.8330989826893003 32 0.47795987745743806 33 -0.91728260527500283 34 -2.3237579403849797
		 35 -3.7125101472865185 36 -5.0545149149126951 37 -6.3206988361320464 38 -7.4819582291792273
		 39 -8.5091755396592372 40 -9.37023984545106 41 -10.031293238206851 42 -10.460618826373047
		 43 -10.6265400462337 44 -10.652176285167915 45 -10.681169537560741 46 -10.712667147362525
		 47 -10.745818662795157 48 -10.779776518981631 49 -10.813696556652042 50 -10.846738344365008
		 51 -10.878065268020219 52 -10.906844351518362 53 -10.932245776230673 54 -10.953442074447333
		 55 -10.969606983159572 56 -10.979913959381133 57 -10.983534376728935 58 -10.975753938203317
		 59 -10.955230948591716 60 -10.926188970567436 61 -10.892859093954637 62 -10.859489583844724
		 63 -10.830348298209278 64 -10.809718645511163 65 -10.801889854468801 66 -10.499880840247936
		 67 -9.660676350900836 68 -8.3845509469796013 69 -6.7717970555755276 70 -4.9226882874335161
		 71 -2.9374560514315169 72 -0.91628053727080283 73 1.0407054027190492 74 2.8334035692928894
		 75 4.3617351897079093 76 5.5271759245852836 77 6.2350753322852119 78 6.7008502408034696
		 79 7.1910975747977561 80 7.6884156901300722 81 8.1754534765094871 82 8.6349039643118264
		 83 9.0494952955224779 84 9.4019790169078306 85 9.6751162782392832 86 9.8516628354907017
		 87 9.914353778276686 88 9.85003708465009 89 9.6727227013240054 90 9.4058752246502451
		 91 9.0729740426217464 92 8.6974957821290193 93 8.3028765554085453 94 7.8910684817756014
		 95 7.4641673427792394 96 6.913759510246039 97 6.1408340575275115 98 5.1829458114568387
		 99 4.0774429277523581 100 2.8596168618335951 101 1.5408484868472501 102 0.14833148719964043
		 103 -1.2728839491009651 104 -2.6941951303723388 105 -4.0868671263747798 106 -5.4220380261109069
		 107 -6.6707352290665289 108 -7.803899568971115 109 -8.7924137589284967 110 -9.6071318031652506
		 111 -10.218906666194638 112 -10.598614603261185 113 -10.717176078654393 114 -10.700301602492603
		 115 -10.694272864731058 116 -10.697928933110125 117 -10.70833240613776 118 -10.724633772824305
		 119 -10.745981509451624 120 -10.771523420099015 121 -10.800407812632935 122 -10.831784529848131
		 123 -10.864805836754805 124 -10.898627150073226 125 -10.932407584829873 126 -10.965310285549091
		 127 -10.996502505888511 128 -11.025155400661195 129 -11.050443497995785 130 -11.071543826881808
		 131 -11.087634686509563 132 -11.097894058622055 133 -11.101497682554538 134 -11.093753287568484
		 135 -11.073324543115453 136 -10.749721755228228 137 -9.8971827237464449 138 -8.6171346356932865
		 139 -7.0111345559980247 140 -5.1808177255570005 141 -3.2278590682882364 142 -1.2371088659901626
		 143 0.70796632423738659 144 2.5078165855507644 145 4.0628953826481524 146 5.2735697644025858
		 147 6.0400529310898454 148 6.5718930557084878 149 7.1297375275255614 150 7.690304960349966
		 151 8.2303184719780216 152 8.7280834889029819 153 9.1658220856105501 154 9.5261694241341619
		 155 9.7917968241663242 156 9.9454033839970997;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_004_cnt_rotateY";
	rename -uid "76E4C6C3-4B9A-E018-DC19-80A1EC61133E";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 1.2105624376644637 18 1.2065292096037348
		 19 1.1941994361127832 20 1.1731685164269352 21 1.1430382518595215 22 1.1035612094552412
		 23 1.0548297835232663 24 0.99790756734498576 25 0.9391989969529283 26 0.86618155861191004
		 27 0.76553598649613053 28 0.64343920750007944 29 0.5059726655732173 30 0.35891221754925751
		 31 0.20755220369886865 32 0.056564479756993066 33 -0.090105616945350617 34 -0.22930367047050104
		 35 -0.35868797786530604 36 -0.47670585141099336 37 -0.58252195988703581 38 -0.67589684517861137
		 39 -0.75701445282157331 40 -0.82519897107406692 41 -0.87877013390599068 42 -0.91626031514151451
		 43 -0.93561870166122163 44 -0.94588068778947931 45 -0.95761458205740491 46 -0.97051671798049133
		 47 -0.98427106783967733 48 -0.99854658571038746 49 -1.0129956268048821 50 -1.0272534398843964
		 51 -1.0409387283911378 52 -1.0536552752387265 53 -1.0649946260646543 54 -1.0745398263560619
		 55 -1.0818702093358568 56 -1.0865672339074219 57 -1.0882213752667267 58 -1.0846692931544644
		 59 -1.0753488646319176 60 -1.0622809933553152 61 -1.0474583448954764 62 -1.0328041555987508
		 63 -1.0201583824038296 64 -1.0112913135825814 65 -1.0079447477774877 66 -0.980973110366954
		 67 -0.90574749342054794 68 -0.7904400928168972 69 -0.64302996255517297 70 -0.47159519713302767
		 71 -0.28450601120350572 72 -0.090519006650230716 73 0.10120869485894937 74 0.28115281036732603
		 75 0.43946268607305161 76 0.56572896672876305 77 0.64822663754691667 78 0.70929815410602537
		 79 0.77792753020221261 80 0.85124463869445455 81 0.9260323752068006 82 0.99885672159067385
		 83 1.0661780250779844 84 1.1244431600071472 85 1.1701583638886961 86 1.1999427142995995
		 87 1.2105624376644637 88 1.2065292096037348 89 1.1941994361127832 90 1.1731685164269352
		 91 1.1430382518595215 92 1.1035612094552405 93 1.0548297835232656 94 1.0019409352536739
		 95 0.94889124737767472 96 0.87984829798968978 97 0.7817157203462155 98 0.65840809804580303
		 99 0.51522148030504111 100 0.35875519850764725 101 0.19495490407065058 102 0.027890001972986769
		 103 -0.13651594620331919 104 -0.29341107103780917 105 -0.43879962674386103 106 -0.56957654948658476
		 107 -0.68351399152165404 108 -0.77919765293763399 109 -0.85591152679299376 110 -0.91347053877069628
		 111 -0.95200140948354484 112 -0.97167284667682674 113 -0.97237689819932693 114 -0.96543226083750211
		 115 -0.96296239631642333 116 -0.96445951546021991 117 -0.96873152589943867 118 -0.97546088480179427
		 119 -0.98433907546079158 120 -0.99505965227601212 121 -1.0073123598573082 122 -1.0207783291261574
		 123 -1.0351263514236246 124 -1.0500102298990397 125 -1.0650672058202917 126 -1.0799174559796791
		 127 -1.0941646561962246 128 -1.1073976052082271 129 -1.1191929031806587 130 -1.1291186797955413
		 131 -1.1367393685699327 132 -1.1416215267206673 133 -1.1433407035385126 134 -1.1396488609416444
		 135 -1.1299598416154806 136 -1.0901868178355696 137 -1.0019180735624522 138 -0.87453796987082943
		 139 -0.71616253255649109 140 -0.53409185561460837 141 -0.33524194170984822 142 -0.12436684860221531
		 143 0.089054364844109904 144 0.29063148043080245 145 0.46590942827234494 146 0.60097037015591592
		 147 0.68298031455449226 148 0.73613829250462803 149 0.79218669910347983 150 0.85076983764879377
		 151 0.91147909629100687 152 0.97326547117240259 153 1.0335859844344388 154 1.0893512257223688
		 155 1.1371944402030729 156 1.1736283889950183;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_004_cnt_rotateZ";
	rename -uid "76B939F2-40A3-2509-D651-2DBDB0BC6FA0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -3.4800238024569641 18 -3.5495335359511846
		 19 -3.7215809678824305 20 -3.9414165317573722 21 -4.154282295394383 22 -4.3054329158670992
		 23 -4.3401415970144832 24 -4.2187500930257169 25 -4.0970218212857654 26 -3.989970534134089
		 27 -3.7655201786998038 28 -3.4414708175338724 29 -3.035671750552225 30 -2.5661280994696307
		 31 -2.0510772991820914 32 -1.5090361272827844 33 -0.9588204115899176 34 -0.4195408620255221
		 35 0.089420478982847287 36 0.54844796842950561 37 0.93773880927807896 38 1.2373529971987596
		 39 1.427262391927175 40 1.5125951402280522 41 1.5134149572128672 42 1.4320960571449908
		 43 1.2711230564283049 44 1.0545389844809623 45 0.80940239731137154 46 0.54285582563072077
		 47 0.26204098649536955 48 -0.025901410255300728 49 -0.3138317869550285 50 -0.59461175164550983
		 51 -0.86110407722117821 52 -1.1061726033915218 53 -1.3226820591207085 54 -1.5034978038957889
		 55 -1.6414854870644437 56 -1.7295106255528307 57 -1.7604381015236557 58 -1.6939786282420082
		 59 -1.5187641228408086 60 -1.2710392511448256 61 -0.98705074843970619 62 -0.70305033032793174
		 63 -0.45529560348314074 64 -0.28004903270384118 65 -0.21357502390789318 66 -0.19387348272501881
		 67 -0.14238538354158198 68 -0.069908700990210043 69 0.013057863725039464 70 0.095743964720326327
		 71 0.16664548972371815 72 0.21317783658258471 73 0.22146526206708891 74 0.17629225307765703
		 75 0.061240702875418281 76 -0.1285031138306004 77 -0.36727382986358509 78 -0.66012236013752768
		 79 -1.0163261052569876 80 -1.4135224729023206 81 -1.8294453164436151 82 -2.2419163418669168
		 83 -2.6288296617423565 84 -2.9681308615662685 85 -3.2377914306243598 86 -3.4157790268829586
		 87 -3.4800238024569641 88 -3.5495335359511846 89 -3.7215809678824305 90 -3.9414165317573722
		 91 -4.154282295394383 92 -4.3054329158671054 93 -4.3401415970144521 94 -4.3728298108635126
		 95 -4.5179890499712636 96 -4.7066042388907254 97 -4.8700898228522389 98 -4.954410636717375
		 99 -4.9055588769734779 100 -4.6847762270542921 101 -4.4489876581219825 102 -4.2289287437601528
		 103 -3.9081699402469066 104 -3.5059072057104714 105 -3.0416506693597931 106 -2.5351764994364174
		 107 -2.0064664857268819 108 -1.4756409822145016 109 -0.96289034527718986 110 -0.48840886979563825
		 111 -0.072333436463878895 112 0.26531335327855998 113 0.50467881777685619 114 0.64752815242871808
		 115 0.69854647942948622 116 0.66760800708466328 117 0.57955310401064353 118 0.44152332855123333
		 119 0.26066086834381252 120 0.044108073687945608 121 -0.20099294090324413 122 -0.46750062846209889
		 123 -0.74827427451764117 124 -1.0361741938794047 125 -1.3240618570463407 126 -1.6047999435560385
		 127 -1.8712523194656108 128 -2.1162839363376302 129 -2.3327606495503406 130 -2.5135489544140843
		 131 -2.651515639413959 132 -2.7395273568961338 133 -2.7704501126681991 134 -2.7040007794865466
		 135 -2.5288129526647012 136 -2.3222980316439346 137 -2.1520264110614864 138 -2.0389453411118268
		 139 -2.0038236474807323 140 -2.0677872484486457 141 -2.2527309115331664 142 -2.4419912308541933
		 143 -2.5130007175433025 144 -2.4806997612613233 145 -2.3614261217300112 146 -2.1725562371264635
		 147 -1.9318938525044089 148 -1.6909268392460406 149 -1.5052065544135516 150 -1.3985134774460861
		 151 -1.3947065685378526 152 -1.5052772009936741 153 -1.7110262817016464 154 -1.9899022473332741
		 155 -2.3198988030037047 156 -2.6790126510190198;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_003_cnt_translateX";
	rename -uid "4EFDD54E-4E60-3062-CEAF-6AB4C1264C52";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 1.905964605291274 18 1.9848593503607219
		 19 2.0700231900279107 20 2.1351701459132642 21 2.1543797124099626 22 2.1056419872510901
		 23 2.0036342107134431 24 1.8777933880720639 25 1.7574384195955872 26 1.6262979555839365
		 27 1.4550262908429374 28 1.253242490076957 29 1.0305734492818033 30 0.79660785387528676
		 31 0.56084508480549289 32 0.33264730706227397 33 0.12120441941510762 34 -0.064478314529708314
		 35 -0.21556127181415263 36 -0.32371354640085315 37 -0.38095478698613761 38 -0.3915711470297083
		 39 -0.36752327559429432 40 -0.31200710331205528 41 -0.22815620918342461 42 -0.11905005531647816
		 43 0.012275760194313534 44 0.15904000251475736 45 0.31390839487573885 46 0.47298456671981626
		 47 0.63229845148819663 48 0.78781182152948759 49 0.9354278735084165 50 1.0710038803767645
		 51 1.190366068834976 52 1.2893261344116809 53 1.3636991706907224 54 1.4093315729394078
		 55 1.4224618419360695 56 1.3928328545013642 57 1.318569567636473 58 1.2087417475161999
		 59 1.0724385691111706 60 0.91872356510924646 61 0.75659572515030504 62 0.59495746650995329
		 63 0.44259170929015568 64 0.30815117314548957 65 0.20016327647834942 66 0.10139747182697079
		 67 -0.0049865825577910527 68 -0.10990004043392787 69 -0.20421903954542131 70 -0.27885812267041388
		 71 -0.32485344829089513 72 -0.33345483851343261 73 -0.2964187149173938 74 -0.21298138351829721
		 75 -0.093469565736455706 76 0.053440876750642019 77 0.21909833633736753 78 0.39951246196588386
		 79 0.59267941487684084 80 0.79296595109360624 81 0.9946779735312532 82 1.192043053609666
		 83 1.3792031959872588 84 1.5502168500585753 85 1.6990688604485911 86 1.8196870654547297
		 87 1.905964605291274 88 1.9848593503607219 89 2.0700231900279107 90 2.1351701459132642
		 91 2.1543797124099626 92 2.1469817593346647 93 2.1358453408650462 94 2.1007136814390606
		 95 2.0213673853945124 96 1.8827810587783915 97 1.7039701204829498 98 1.5157574390734965
		 99 1.3487834830710028 100 1.1877385818067978 101 1.0034047353877611 102 0.80533727313846271
		 103 0.60297203752597284 104 0.40558319320138025 105 0.22226257704906516 106 0.06151311424619621
		 107 -0.068627142372406524 108 -0.16361765831217667 109 -0.2220815668725038 110 -0.24045933483822068
		 111 -0.21507068372710592 112 -0.15105030018935395 113 -0.058082405836387352 114 0.057076124568794739
		 115 0.18714959733438263 116 0.32831028788041294 117 0.47666195981983606 118 0.62823902305001411
		 119 0.7790108403934255 120 0.92489021023669693 121 1.0617450678709019 122 1.1854125714761494
		 123 1.2917149718861367 124 1.3764853020970804 125 1.4359255760748511 126 1.4665138467444763
		 127 1.4675643151716713 128 1.4397702937574763 129 1.383871880266355 130 1.2938790540862328
		 131 1.1693601043067758 132 1.0194010916198977 133 0.85302693433399668 134 0.67916613483291144
		 135 0.50662394977831582 136 0.35149671782676251 137 0.22819979528239287 138 0.14303082298997083
		 139 0.10241603402249666 140 0.079941091329231995 141 0.050955186816537434 142 0.024415219169128477
		 143 0.0087190724575521017 144 0.011643760747375609 145 0.037954636847416623 146 0.09115274827249209
		 147 0.17477711912857785 148 0.29012599377068682 149 0.43234554213398724 150 0.59588426468215516
		 151 0.77517208479474675 152 0.96458333177045574 153 1.1584117184872866 154 1.3508571282048649
		 155 1.5360232215236067 156 1.7079243883628124;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_003_cnt_translateY";
	rename -uid "B69184DF-474F-2C89-2CA3-21A0FBBF1C98";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0.47420079695569939 18 0.50048068230207576
		 19 0.5263105470915761 20 0.54618383035417395 21 0.55517226307298273 22 0.54895618096674781
		 23 0.53097757534104773 24 0.50807302728037484 25 0.48754412889044119 26 0.46520681603974623
		 27 0.43185776346833649 28 0.38798733339951497 29 0.33454729782076953 30 0.27312050605132043
		 31 0.20602315034394536 32 0.13633958047554273 33 0.067890480234538586 34 0.0051360445709178748
		 35 -0.046983683814367794 36 -0.083894311784192155 37 -0.10159722949718031 38 -0.10101723286823017
		 39 -0.086526418798200666 40 -0.059186864324949795 41 -0.020137768493107444 42 0.029395597977085686
		 43 0.088066762183643732 44 0.15185736670023431 45 0.21642100605465231 46 0.27994145394050918
		 47 0.34081051864850309 48 0.3976315208573169 49 0.4492092430328114 50 0.49452607847159413
		 51 0.53270418817220389 52 0.56295356825078358 53 0.58450603335226248 54 0.59654204523999965
		 55 0.59837341631738639 56 0.58764448446574846 57 0.56375846806846397 58 0.52846502664475281
		 59 0.48348152317625903 60 0.43067888182390845 61 0.37221841978436743 62 0.31063967779662249
		 63 0.2488996788208695 64 0.19036437513896942 65 0.13875325908487923 66 0.092173728159693269
		 67 0.046708955520223583 68 0.0028607708866772441 69 -0.038796097670022789 70 -0.077045892768232704
		 71 -0.10967174964488891 72 -0.13334321347302591 73 -0.14411585226326196 74 -0.14004991555721347
		 75 -0.12287880109970217 76 -0.093110168113462066 77 -0.050646925416835131 78 0.0021454097283708506
		 79 0.060866661856465498 80 0.12291809908623463 81 0.18586153655522253 82 0.24747570785311979
		 83 0.30578457934991121 84 0.35905714963070068 85 0.40577819508825996 86 0.44458944683137247
		 87 0.47420079695569939 88 0.50048068230207576 89 0.5263105470915761 90 0.54618383035417395
		 91 0.55517226307298273 92 0.5582923960623063 93 0.56141847447173632 94 0.56073573510187913
		 95 0.55180233745580409 96 0.52993350893142122 97 0.4973339317133636 98 0.46005971456511929
		 99 0.4255719340405264 100 0.38948562105750284 101 0.34284871505149539 102 0.28784370663890968
		 103 0.22736413375713482 104 0.16501923014372011 105 0.10507321196524089 106 0.051743088132518267
		 107 0.0090336859580304463 108 -0.020382168366694486 109 -0.035463852676159036 110 -0.034401872715839943
		 111 -0.015690506451690567 112 0.018778866766361091 113 0.065371462946373526 114 0.11998726393054682
		 115 0.1781322915060386 116 0.23779184999043679 117 0.29712587544619851 118 0.35448996004581801
		 119 0.40844318875894459 120 0.4577424763827409 121 0.50132314296738656 122 0.53826554035844509
		 123 0.56774763289819674 124 0.58899041836328081 125 0.60145731939292801 126 0.60462718656268777
		 127 0.59865124532962 128 0.58387841578213795 129 0.56046838730863158 130 0.52674434934692016
		 131 0.48209316483116282 132 0.42826422244799289 133 0.36727553766772303 134 0.30151846168216423
		 135 0.2338136587726467 136 0.17229007797995166 137 0.12370030634027529 138 0.088647765026180991
		 139 0.067550752409637482 140 0.049703114431608242 141 0.025094310480277215 142 -0.003698383669359373
		 143 -0.033117417274247885 144 -0.058554236678858729 145 -0.075633027796740748 146 -0.07976246247883978
		 147 -0.065994452702454964 148 -0.034696871244044303 149 0.0085967200070271588 150 0.060980178034050425
		 151 0.1195694392797364 152 0.18160764056226242 153 0.24454276603719904 154 0.30607758975745014
		 155 0.36419142581735287 156 0.41713307346386586;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_003_cnt_translateZ";
	rename -uid "815773BB-486E-2C1A-832C-239DF294E870";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 2.3671385202550432 18 2.3545622586274533
		 19 2.3320118233470062 20 2.2973263587706789 21 2.2482924679084011 22 2.1822095694889825
		 23 2.0918535830802512 24 1.9681673353047167 25 1.8020849517276645 26 1.5945727835781582
		 27 1.3557706919818493 28 1.0903790028272589 29 0.80312026417068694 30 0.49874572402706363
		 31 0.18203921850170346 32 -0.14218196244103609 33 -0.46906962486827197 34 -0.793750901821344
		 35 -1.1113382164777867 36 -1.416889980595581 37 -1.7054348014254899 38 -1.9704978654268928
		 39 -2.2046698077157387 40 -2.4016475156025798 41 -2.55514762298791 42 -2.658896401775813
		 43 -2.7066171401763839 44 -2.7248832448988267 45 -2.7441397064967177 46 -2.7639007717819766
		 47 -2.7836731112378992 48 -2.8029565150136406 49 -2.821244991409344 50 -2.8380281464271064
		 51 -2.8527927408641709 52 -2.8650243528338848 53 -2.8742091185837459 54 -2.8798366383373093
		 55 -2.8814450535539038 56 -2.8777717510990151 57 -2.8685848157653986 58 -2.8549984858982658
		 59 -2.8381291446783035 60 -2.8190910787286594 61 -2.7989926585246305 62 -2.7789330284366152
		 63 -2.7599995807337798 64 -2.7432665975377173 65 -2.72979547822756 66 -2.6517254375305939
		 67 -2.4542853220068057 68 -2.1581333079186695 69 -1.7839460610731361 70 -1.3524670030024772
		 71 -0.88452008448294706 72 -0.40098802637024455 73 0.077263864541468052 74 0.53030391828253443
		 75 0.9396342637078412 76 1.286643707989473 77 1.5528160057522555 78 1.7542409513643946
		 79 1.9212546859080284 80 2.0567622607981271 81 2.1636680454807617 82 2.2448763544522663
		 83 2.3032919288832439 84 2.3418201391629871 85 2.3633668548712787 86 2.3708379647440641
		 87 2.3671385202550432 88 2.3545622586274533 89 2.3320118233470062 90 2.2973263587706789
		 91 2.2482924679084011 92 2.1771218493894722 93 2.0755783793470286 94 1.9407169400574773
		 95 1.7695720699221731 96 1.5629619781770678 97 1.3250714510902242 98 1.0579860794630846
		 99 0.76383053051521088 100 0.45042260527727507 101 0.12732789969674485 102 -0.20065378426002667
		 103 -0.52869798893649644 104 -0.85195888098677131 105 -1.1655750527246305 106 -1.4646234525722228
		 107 -1.7441397438617656 108 -1.9987460844061218 109 -2.2226905256791412 110 -2.4105103021893903
		 111 -2.5567679873182314 112 -2.6549355723276733 113 -2.6979128638620011 114 -2.7122756872525464
		 115 -2.7284751740164772 116 -2.7460325969476962 117 -2.7644620607137327 118 -2.7832705392600694
		 119 -2.8019584514594662 120 -2.8200206546995794 121 -2.8369477382897195 122 -2.8522275140454525
		 123 -2.8653466303600545 124 -2.875793332465495 125 -2.883102333930001 126 -2.8868420088220255
		 127 -2.8869299900766592 128 -2.8834528827770374 129 -2.87650172181438 130 -2.8653371565329451
		 131 -2.8499026283135311 132 -2.8313143546190691 133 -2.8106829054956055 134 -2.7891096114118596
		 135 -2.7676836736689037 136 -2.682662459362346 137 -2.4831454804365123 138 -2.1894522627321358
		 139 -1.821930482517887 140 -1.3969286171480224 141 -0.9311028188019641 142 -0.44535593270523677
		 143 0.039430547180781872 144 0.50245460406569364 145 0.92334301968768884 146 1.2819703648031382
		 147 1.5583061378432745 148 1.7677839218288369 149 1.9410938640908668 150 2.081133121854144
		 151 2.1907939513282209 152 2.2729663859759626 153 2.3305405525770233 154 2.366408391316849
		 155 2.3834646873154561 156 2.3846074191919389;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_003_cnt_rotateX";
	rename -uid "1C05B130-49E5-BDEC-D5BA-53B2656EEE2C";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 4.3770415661627737 18 4.3364720364657741
		 19 4.281955471890547 20 4.2101148411529881 21 4.1168135857475683 22 3.9966756913790817
		 23 3.8348627310136356 24 3.6124487698024281 25 3.3104699053316766 26 2.9298188703624755
		 27 2.4896390481694262 28 1.9982912093569178 29 1.4641165191145178 30 0.89543738345389357
		 31 0.30056188239507375 32 -0.31220956542286826 33 -0.93457680728594572 34 -1.5582332418080451
		 35 -2.174860869165367 36 -2.7752909126866983 37 -3.3496127833078186 38 -3.8844476742609975
		 39 -4.3637895006285641 40 -4.7742452978153418 41 -5.1024187778722299 42 -5.334913563791444
		 43 -5.458337086693553 44 -5.5246266523033327 45 -5.5934755099373641 46 -5.6631335709475419
		 47 -5.7318562193141664 48 -5.7979059924968341 49 -5.8595537110904994 50 -5.9150787779172882
		 51 -5.9627684138815118 52 -6.000915674446837 53 -6.027816196544701 54 -6.0417780131986012
		 55 -6.0416969282955257 56 -6.0251217976733003 57 -5.9921076076525308 58 -5.945578291906676
		 59 -5.8884585600070984 60 -5.8236818840867679 61 -5.7541951988834956 62 -5.6829604651033545
		 63 -5.6129536342032953 64 -5.5471617923871337 65 -5.4885793508392142 66 -5.3031193920086235
		 67 -4.8828106898089718 68 -4.2698521428807794 69 -3.5064578187816067 70 -2.6348336767885558
		 71 -1.6971598501564298 72 -0.7355251673805222 73 0.20866326946332309 74 1.0961832382608041
		 75 1.8912120550480775 76 2.5577657746260622 77 3.0598819508374584 78 3.4302537344293684
		 79 3.729969608364081 80 3.9656609401828029 81 4.1439485909033316 82 4.2714399140746968
		 83 4.3547261390218299 84 4.4003807885893993 85 4.4149597424228029 86 4.4050034441106867
		 87 4.3770415661627737 88 4.3364720364657741 89 4.281955471890547 90 4.2101148411529881
		 91 4.1168135857475683 92 3.9866828542401076 93 3.8028597207862891 94 3.5583861449235408
		 95 3.2463163635487629 96 2.8672992457684403 97 2.4287486118960606 98 1.9338405832787551
		 99 1.3856984401447754 100 0.79868394659237452 101 0.1906560003117522 102 -0.43005662546557449
		 103 -1.0551315678573097 104 -1.6762478502090092 105 -2.2850807189790436 106 -2.8724612090838222
		 107 -3.4284823514497655 108 -3.9420254576974103 109 -4.4005080738454003 110 -4.7922864523534674
		 111 -5.1057119789404979 112 -5.3268809145580445 113 -5.440730879820217 114 -5.4992003888736871
		 115 -5.5619821581914284 116 -5.6273240339183621 117 -5.6934775998640248 118 -5.758699973999116
		 119 -5.8212553221737418 120 -5.87941580867132 121 -5.9314617195294064 122 -5.9756805411044756
		 123 -6.0103648527119162 124 -6.033823252799241 125 -6.0449565968925745 126 -6.0429696897827672
		 127 -6.028246704679928 128 -6.0017285319432787 129 -5.96434934639659 130 -5.9153894870659816
		 131 -5.8554594156705493 132 -5.787493345513246 133 -5.7144394267070266 134 -5.6392612500589125
		 135 -5.5649366757683554 136 -5.3656441481265178 137 -4.9412809865285281 138 -4.3334339124717216
		 139 -3.5836969328013013 140 -2.7253679982733523 141 -1.7921317837576134 142 -0.82607293813585436
		 143 0.13140228263061635 144 1.0393027222536673 145 1.8579513381802966 146 2.5482341509811413
		 147 3.0710602895802199 148 3.457762938706173 149 3.7701533154069162 150 4.0148646794670011
		 151 4.1985243300714234 152 4.3277509368835574 153 4.4091514811224224 154 4.4493183495611746
		 155 4.4548271998315787 156 4.4322362164574454;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_003_cnt_rotateY";
	rename -uid "3389B98D-49D2-493E-5F97-32A8A90BA57D";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0.61771647679178587 18 0.610908681640284
		 19 0.6021036488357201 20 0.59118520529974206 21 0.57794709681820344 22 0.56185857592653699
		 23 0.54049877148312264 24 0.51072255777544184 25 0.4696020091401954 26 0.41738494098771706
		 27 0.3567579721105002 28 0.28873792579819507 29 0.21443726638520874 30 0.13508580949304944
		 31 0.052038110358975043 32 -0.033233617911589824 33 -0.11916112179983068 34 -0.20411622644600796
		 35 -0.28646133259162893 36 -0.36458182359546731 37 -0.43697531405639367 38 -0.50220662364627933
		 39 -0.55892105566766137 40 -0.60596212293673568 41 -0.64222106408513924 42 -0.66658637831271828
		 43 -0.67789309530260633 44 -0.68270182427004478 45 -0.68836804207805125 46 -0.69479921124528998
		 47 -0.70183425099682173 48 -0.70924369478082883 49 -0.71673420569494739 50 -0.72395741253524426
		 51 -0.73052302534368141 52 -0.73601619363307624 53 -0.74001908446009346 54 -0.74213886009058705
		 55 -0.74212646589834763 56 -0.73961332498820886 57 -0.73472876786457786 58 -0.72811780473576837
		 59 -0.72043794918278103 60 -0.71230796897779958 61 -0.70426904727715955 62 -0.69675841071074684
		 63 -0.69009553082668085 64 -0.68448101470363154 65 -0.6800082790808688 66 -0.66067613999859987
		 67 -0.61349856455629836 68 -0.54277130450986644 69 -0.45261791617878627 70 -0.34717410953035582
		 71 -0.23074304023028111 72 -0.10791997271837393 73 0.016329268610986179 74 0.13666844513683143
		 75 0.24748995370535015 76 0.34291548225918073 77 0.41694107026989041 78 0.47316618271065741
		 79 0.51945394569898573 80 0.55628814529103654 81 0.58428903042720948 82 0.60421526259327363
		 83 0.61695874814196761 84 0.62353230967313711 85 0.62505020225176644 86 0.62270151152294118
		 87 0.61771647679178587 88 0.610908681640284 89 0.6021036488357201 90 0.59118520529974206
		 91 0.57794709681820344 92 0.56006600586187127 93 0.53490988580723764 94 0.50162910843255082
		 95 0.45931011603003497 96 0.40794982940157276 97 0.34825472026991833 98 0.28051907080154398
		 99 0.20538651944667108 100 0.12508318432930637 101 0.042012756245682535 102 -0.042592420406372872
		 103 -0.12741485848505057 104 -0.21106213735773249 105 -0.29209132893755868 106 -0.36900361411719601
		 107 -0.44031962019858967 108 -0.50460816061064229 109 -0.56050262507534099 110 -0.60679920501515805
		 111 -0.64239097213459517 112 -0.6661172380812127 113 -0.67672240383979865 114 -0.68078242367788699
		 115 -0.68569111061335919 116 -0.69140525289922961 117 -0.69782084288046853 118 -0.70476884696227027
		 119 -0.71201535461745213 120 -0.71926607882834503 121 -0.72617517200158799 122 -0.73235831874876467
		 123 -0.73741007224493849 124 -0.74092757775649887 125 -0.74262548843731246 126 -0.7423211255682608
		 127 -0.74008401582299743 128 -0.73613558211347574 129 -0.73074641395491735 130 -0.72399910611125062
		 131 -0.7162194835860406 132 -0.70803335121359823 133 -0.69998635068652493 134 -0.69251597299741752
		 135 -0.68593614580473306 136 -0.66497566120541529 137 -0.61704976094009456 138 -0.54619922111155395
		 139 -0.45637713945868408 140 -0.3512283090499318 141 -0.23473727474788358 142 -0.11162493537142007
		 143 0.013107869741716717 144 0.13413437678185988 145 0.24584958433810458 146 0.34238348367515886
		 147 0.41765488498233311 148 0.47518202512933355 149 0.52282190213014257 150 0.56096923181539471
		 151 0.59012425922134049 152 0.61090685920671284 153 0.62406352878349325 154 0.6304671714600929
		 155 0.63110963193931335 156 0.62708699155313219;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_003_cnt_rotateZ";
	rename -uid "5A50A23A-4429-41F9-D2A8-9889A4D674D6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -6.4812938962137334 18 -6.7731922376187246
		 19 -7.0287996879870711 20 -7.1866553620124707 21 -7.1915700526803867 22 -6.9988544909355914
		 23 -6.6431385407815755 24 -6.1931444540550071 25 -5.7175327069246231 26 -5.1925877337516049
		 27 -4.5675788079674096 28 -3.8705888564539794 29 -3.1297222366931319 30 -2.3731135266048891
		 31 -1.6289341340551529 32 -0.92539695261481802 33 -0.29075931135833089 34 0.24667554989390905
		 35 0.658558295550835 36 0.92336641292992561 37 1.0254983491211827 38 0.97766812304067041
		 39 0.81401740625016461 40 0.54713962931310067 41 0.18961121962356425 42 -0.24598692914557144
		 43 -0.74704491193884781 44 -1.292119827351996 45 -1.8586581021792385 46 -2.4323460219272959
		 47 -2.9988704769537082 48 -3.5439191435202875 49 -4.0531806025696611 50 -4.5123443666709679
		 51 -4.9071007914114917 52 -5.2231408562315078 53 -5.4461558109876425 54 -5.5619549248529738
		 55 -5.5612823064533856 56 -5.4238125220601372 57 -5.1501462828711331 58 -4.7647649494728004
		 59 -4.2921499546874218 60 -3.7567836670040968 61 -3.1831499061104482 62 -2.5957341226802222
		 63 -2.0190232957331902 64 -1.477505628295203 65 -0.99567013422280148 66 -0.5472689464778917
		 67 -0.10580539913512127 68 0.30476135130760007 69 0.66054013956930968 70 0.93755746590615174
		 71 1.1116372348516623 72 1.1587510603928297 73 1.059761325391172 74 0.81275277829411052
		 75 0.44309740985352208 76 -0.025670371448062938 77 -0.57015996124465484 78 -1.1748564270544277
		 79 -1.8272160335737209 80 -2.5082975231785478 81 -3.1991122071157996 82 -3.8806335372579621
		 83 -4.5338059532664792 84 -5.1395528219268876 85 -5.6787833163397137 86 -6.1323981213275429
		 87 -6.4812938962137334 88 -6.7731922376187246 89 -7.0287996879870711 90 -7.1866553620124707
		 91 -7.1915700526803867 92 -7.0823344542793603 93 -6.9102616934984349 94 -6.6438855884780246
		 95 -6.2516984274633378 96 -5.7123444706823712 97 -5.0729206574079502 98 -4.4045547521085853
		 99 -3.778369184260022 100 -3.1722548939197486 101 -2.5354903810092919 102 -1.89631163157294
		 103 -1.2830014376385406 104 -0.72388952963161091 105 -0.24735126780441319 106 0.12506676968493255
		 107 0.37774250483359301 108 0.50486619230995222 109 0.51252352969044224 110 0.39899888317461574
		 111 0.16256524447480492 112 -0.17999643443958871 113 -0.60233567065316196 114 -1.0830048965421188
		 115 -1.599451521741015 116 -2.137361600845042 117 -2.6824216034479313 118 -3.2203186095829186
		 119 -3.736740472239239 120 -4.2173759166162013 121 -4.6479145482986741 122 -5.0140467482554163
		 123 -5.3014634411423076 124 -5.4959739614726812 125 -5.5883229647464763 126 -5.5718403005059658
		 127 -5.4497259173974522 128 -5.2298778786990132 129 -4.9201935200275431 130 -4.5149151133641201
		 131 -4.0193413274079894 132 -3.4579546060388688 133 -2.8552388974677605 134 -2.2356798283063362
		 135 -1.6237645862729662 136 -1.0612711986041101 137 -0.58628038868115695 138 -0.21759822900657835
		 139 0.026017230291570402 140 0.19377137837387187 141 0.33128439388073838 142 0.41458667521617676
		 143 0.42460587746579276 144 0.34496843119464987 145 0.169438492851652 146 -0.10413554753070942
		 147 -0.47808099832494538 148 -0.94808443767136019 149 -1.4956587150896297 150 -2.1018777490998497
		 151 -2.7477639030814061 152 -3.4142978004212443 153 -4.0824275563472598 154 -4.7330772489571142
		 155 -5.3471544658310615 156 -5.9055567854368718;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_002_cnt_translateX";
	rename -uid "A27D5477-4571-E45B-704F-D38811C2D736";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0.11198580231780397 18 0.11559192871249024
		 19 0.11686090163806284 20 0.11536129652375848 21 0.11113096344294604 22 0.10458027691498728
		 23 0.096125052235208841 24 0.086181348321701989 25 0.07516111928757141 26 0.063468692004818195
		 27 0.051498086286443367 28 0.039631223390813375 29 0.028237087367159575 30 0.017671912514657606
		 31 0.0082804691847684353 32 0.00039850967394272629 33 -0.0056435836621631097 34 -0.0095159344521960065
		 35 -0.010883116203046939 36 -0.0098533463414298694 37 -0.0069175334382407527 38 -0.0023008030787821099
		 39 0.0037756672060140772 40 0.011091463602980411 41 0.019424272210670779 42 0.028547750750902878
		 43 0.038229937045912266 44 0.048232184388808719 45 0.05830860543289873 46 0.068206000370764741
		 47 0.07766424203757083 48 0.086417090237688399 49 0.094193410141656386 50 0.10071877511271055
		 51 0.10571744284393958 52 0.10891470525299951 53 0.1100396271651789 54 0.10883678119537876
		 55 0.10541837197214932 56 0.10007494744078826 57 0.093101379179103105 58 0.084793735995418729
		 59 0.075446598687051392 60 0.065350797604708077 61 0.054791572510055175 62 0.044047168355831445
		 63 0.033387891027501837 64 0.023075653801726048 65 0.013364048362291214 66 0.0044989737734795199
		 67 -0.0032801470457286541 68 -0.0097385409703747428 69 -0.014643636447601693 70 -0.017761994968140016
		 71 -0.018855640528983031 72 -0.017706809424936409 73 -0.014430797969822606 74 -0.0092768314613635994
		 75 -0.002489136535501757 76 0.0056890044422743813 77 0.015011865955528947 78 0.025228580117442334
		 79 0.036081127497425314 80 0.047303004126746373 81 0.058618538765045969 82 0.069742826181794726
		 83 0.080382237710580284 84 0.09023546979568664 85 0.098995094815506945 86 0.10634958624714841
		 87 0.11198580231780397 88 0.11559192871249024 89 0.11686090163806284 90 0.11536129652375848
		 91 0.11113096344294604 92 0.10458027691498728 93 0.096125052235208841 94 0.086181348321701989
		 95 0.07516111928757141 96 0.063468692004818195 97 0.051498086286443367 98 0.039631223390813375
		 99 0.028237087367159575 100 0.017671912514657606 101 0.0082804691847684353 102 0.00039850967394272629
		 103 -0.0056435836621631097 104 -0.0095159344521960065 105 -0.010883116203046939 106 -0.0098533463414298694
		 107 -0.0069175334382407527 108 -0.0023008030787821099 109 0.0037756672060140772 110 0.011091463602980411
		 111 0.019424272210670779 112 0.028547750750902878 113 0.038229937045912266 114 0.048232184388808719
		 115 0.05830860543289873 116 0.068206000370764741 117 0.07766424203757083 118 0.086417090237688399
		 119 0.094193410141656386 120 0.10071877511271055 121 0.10571744284393958 122 0.10891470525299951
		 123 0.1100396271651789 124 0.10883678119537876 125 0.10541837197214932 126 0.10007494744078826
		 127 0.093101379179103105 128 0.084793735995418729 129 0.075446598687051392 130 0.065350797604708077
		 131 0.054791572510055175 132 0.044047168355831445 133 0.033387891027501837 134 0.023075653801726048
		 135 0.013364048362291214 136 0.0044989737734795199 137 -0.0032801470457286541 138 -0.0097385409703747428
		 139 -0.014643636447601693 140 -0.017761994968140016 141 -0.018855640528983031 142 -0.017706809424936409
		 143 -0.014430797969822606 144 -0.0092768314613635994 145 -0.002489136535501757 146 0.0056890044422743813
		 147 0.015011865955528947 148 0.025228580117442334 149 0.036081127497425314 150 0.047303004126746373
		 151 0.058618538765045969 152 0.069742826181794726 153 0.080382237710580284 154 0.09023546979568664
		 155 0.098995094815506945 156 0.10634958624714841;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_002_cnt_translateY";
	rename -uid "CE83AB49-428D-C598-DE72-5CAEC71AADD9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0.1407435949016147 18 0.14488842477415176
		 19 0.14634170532132629 20 0.14462400190326719 21 0.13975780075529798 22 0.13216186930364415
		 23 0.12224713149521449 24 0.11042492293874062 25 0.097113692981643851 26 0.082744138795106892
		 27 0.067762777955458375 28 0.052633982948023572 29 0.037840513381716789 30 0.02388258908077745
		 31 0.011275549071562807 32 0.00054613721248131242 33 -0.0077725557248982113 34 -0.013147481666095473
		 35 -0.015053369745494649 36 -0.013617443445902211 37 -0.0095370549594377962 38 -0.0031600900591826075
		 39 0.0051601552248499161 40 0.015069029459745309 41 0.026213977771746499 42 0.038247400086518724
		 43 0.050828915450516376 44 0.063627024935115628 45 0.076320160429148132 46 0.088597103471933281
		 47 0.10015675707307281 48 0.11070725406199955 49 0.11996438776451157 50 0.12764935456350912
		 51 0.13348580306664815 52 0.13719619110708692 53 0.1384974596729478 54 0.13710597079005282
		 55 0.13313783030711335 56 0.12689444859766041 57 0.11867093366364401 58 0.10876104057072666
		 59 0.097461338063077108 60 0.085074581195151211 61 0.071912288418722881 62 0.058296529513256701
		 63 0.044560937086849606 64 0.031050959017449031 65 0.018123372009142713 66 0.0061450772203031079
		 67 -0.0045088025202417725 68 -0.013457508241351945 69 -0.020317903813165117 70 -0.024708408090539535
		 71 -0.026253611475361538 72 -0.024630511413228362 73 -0.02001906495753758 74 -0.012814606984626664
		 75 -0.0034192873864427042 76 0.0077630202746235 77 0.020331056802895375 78 0.033889775115881093
		 79 0.048053197297292627 80 0.062446520138699668 81 0.076707450966495117 82 0.090486751136950261
		 83 0.10344796295493808 84 0.11526629672568589 85 0.12562665793442562 86 0.13422079994150948
		 87 0.1407435949016147 88 0.14488842477415176 89 0.14634170532132629 90 0.14462400190326719
		 91 0.13975780075529798 92 0.13216186930364415 93 0.12224713149521449 94 0.11042492293874062
		 95 0.097113692981643851 96 0.082744138795106892 97 0.067762777955458375 98 0.052633982948023572
		 99 0.037840513381716789 100 0.02388258908077745 101 0.011275549071562807 102 0.00054613721248131242
		 103 -0.0077725557248982113 104 -0.013147481666095473 105 -0.015053369745494649 106 -0.013617443445902211
		 107 -0.0095370549594377962 108 -0.0031600900591826075 109 0.0051601552248499161 110 0.015069029459745309
		 111 0.026213977771746499 112 0.038247400086518724 113 0.050828915450516376 114 0.063627024935115628
		 115 0.076320160429148132 116 0.088597103471933281 117 0.10015675707307281 118 0.11070725406199955
		 119 0.11996438776451157 120 0.12764935456350912 121 0.13348580306664815 122 0.13719619110708692
		 123 0.1384974596729478 124 0.13710597079005282 125 0.13313783030711335 126 0.12689444859766041
		 127 0.11867093366364401 128 0.10876104057072666 129 0.097461338063077108 130 0.085074581195151211
		 131 0.071912288418722881 132 0.058296529513256701 133 0.044560937086849606 134 0.031050959017449031
		 135 0.018123372009142713 136 0.0061450772203031079 137 -0.0045088025202417725 138 -0.013457508241351945
		 139 -0.020317903813165117 140 -0.024708408090539535 141 -0.026253611475361538 142 -0.024630511413228362
		 143 -0.02001906495753758 144 -0.012814606984626664 145 -0.0034192873864427042 146 0.0077630202746235
		 147 0.020331056802895375 148 0.033889775115881093 149 0.048053197297292627 150 0.062446520138699668
		 151 0.076707450966495117 152 0.090486751136950261 153 0.10344796295493808 154 0.11526629672568589
		 155 0.12562665793442562 156 0.13422079994150948;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_002_cnt_translateZ";
	rename -uid "511C14DA-4B77-028D-58E6-3A803AA9FAC9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -0.014691993350449906 18 -0.015162113713238057
		 19 -0.015327505539101338 20 -0.015132051962428505 21 -0.014580525367559716 22 -0.013726017230712984
		 23 -0.012622219657334455 24 -0.011322857528089258 25 -0.0098812117987669978 26 -0.0083497310768692046
		 27 -0.0067797335449064633 28 -0.005221204964648507 29 -0.0037227008774003423 30 -0.0023313622364362629
		 31 -0.0010930535946833686 32 -5.263166499247518e-05 33 0.00074565039865603921 34 0.0012576012519680546
		 35 0.0014384151242836651 36 0.0013022219033906879 37 0.00091404602340006136 38 0.00030392353133734673
		 39 -0.00049854753589784195 40 -0.0014638514900546795 41 -0.002562257930666334 42 -0.0037635851764470374
		 43 -0.0050370243391739677 44 -0.0063510227824998822 45 -0.00767322463550002 46 -0.0089704652945741259
		 47 -0.010208816457140557 48 -0.011353678194641503 49 -0.012369914898670231 50 -0.013222032633855463
		 51 -0.013874396505073605 52 -0.014291488103385319 53 -0.014438204926840115 54 -0.014281324314122656
		 55 -0.013835374923623078 56 -0.013137983808956477 57 -0.012227254792149367 58 -0.011141426737756532
		 59 -0.0099185793963378899 60 -0.0085963845013790063 61 -0.0072119020451140159 62 -0.0058014234386440933
		 63 -0.004400364575291249 64 -0.0030432126682065075 65 -0.0017635311294181832 66 -0.00059402671038988331
		 67 0.00043331735588303388 68 0.0012870393714585759 69 0.0019359268199359292 70 0.00234867383055537
		 71 0.0024934707700179359 72 0.002341367926764093 73 0.0019077618271765573 74 0.0012259825687763737
		 75 0.00032880541169078015 76 -0.00075109583218235798 77 -0.001980770644415486 78 -0.0033266915600762559
		 79 -0.0047545320632202959 80 -0.0062290208497763899 81 -0.0077138691681568616 82 -0.0091717669020709991
		 83 -0.010564442501316051 84 -0.011852781808141089 85 -0.012997001285206977 86 -0.013956872138194854
		 87 -0.014691993350449906 88 -0.015162113713238057 89 -0.015327505539101338 90 -0.015132051962428505
		 91 -0.014580525367559716 92 -0.013726017230712984 93 -0.012622219657334455 94 -0.011322857528089258
		 95 -0.0098812117987669978 96 -0.0083497310768692046 97 -0.0067797335449064633 98 -0.005221204964648507
		 99 -0.0037227008774003423 100 -0.0023313622364362629 101 -0.0010930535946833686 102 -5.263166499247518e-05
		 103 0.00074565039865603921 104 0.0012576012519680546 105 0.0014384151242836651 106 0.0013022219033906879
		 107 0.00091404602340006136 108 0.00030392353133734673 109 -0.00049854753589784195
		 110 -0.0014638514900546795 111 -0.002562257930666334 112 -0.0037635851764470374 113 -0.0050370243391739677
		 114 -0.0063510227824998822 115 -0.00767322463550002 116 -0.0089704652945741259 117 -0.010208816457140557
		 118 -0.011353678194641503 119 -0.012369914898670231 120 -0.013222032633855463 121 -0.013874396505073605
		 122 -0.014291488103385319 123 -0.014438204926840115 124 -0.014281324314122656 125 -0.013835374923623078
		 126 -0.013137983808956477 127 -0.012227254792149367 128 -0.011141426737756532 129 -0.0099185793963378899
		 130 -0.0085963845013790063 131 -0.0072119020451140159 132 -0.0058014234386440933
		 133 -0.004400364575291249 134 -0.0030432126682065075 135 -0.0017635311294181832 136 -0.00059402671038988331
		 137 0.00043331735588303388 138 0.0012870393714585759 139 0.0019359268199359292 140 0.00234867383055537
		 141 0.0024934707700179359 142 0.002341367926764093 143 0.0019077618271765573 144 0.0012259825687763737
		 145 0.00032880541169078015 146 -0.00075109583218235798 147 -0.001980770644415486
		 148 -0.0033266915600762559 149 -0.0047545320632202959 150 -0.0062290208497763899
		 151 -0.0077138691681568616 152 -0.0091717669020709991 153 -0.010564442501316051 154 -0.011852781808141089
		 155 -0.012997001285206977 156 -0.013956872138194854;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_002_cnt_rotateX";
	rename -uid "577C0D73-42AA-0D86-37FD-6B8DEE8EA723";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -0.38911947586175744 18 -0.4011197743268633
		 19 -0.40533549661986684 20 -0.40035318268067688 21 -0.38627038235769023 22 -0.36438110722368011
		 23 -0.33597811398389216 24 -0.30235469026612943 25 -0.2648059906935446 26 -0.22462989200470573
		 27 -0.18312738719376109 28 -0.1416025771948414 29 -0.10136234449209883 30 -0.063715806116563412
		 31 -0.029973643712996829 32 -0.0014473956464669574 33 0.020551229567608811 34 0.034710997899665502
		 35 0.039721783539431427 36 0.035947068057327676 37 0.02520430322380485 38 0.0083662978128091047
		 39 -0.013693444473903216 40 -0.040101049107409283 41 -0.069982587591595841 42 -0.10246440724136138
		 43 -0.13667347493055795 44 -0.17173772058692333 45 -0.20678635477617219 46 -0.24095012712520783
		 47 -0.27336148857808795 48 -0.30315462054846209 49 -0.32946529791019946 50 -0.3514305604320489
		 51 -0.36818817869235759 52 -0.37887591569234524 53 -0.38263060431720525 54 -0.37861571689330542
		 55 -0.36718722621045635 56 -0.34926787803513332 57 -0.32577936745451669 58 -0.29764340040240345
		 59 -0.26578254025725884 60 -0.23112081486954827 61 -0.19458408251995057 62 -0.15710017387401831
		 63 -0.11959884097831056 64 -0.083011553720371958 65 -0.048271188922635044 66 -0.016311657328846148
		 67 0.011932490849560471 68 0.035526450445280994 69 0.053535518509578828 70 0.065025425435123768
		 71 0.069062664680989524 72 0.06482181046672407 73 0.052752456770834288 74 0.033835293170691706
		 75 0.0090518622767342405 76 -0.020615798334162133 77 -0.054185519519103692 78 -0.090675397876649924
		 79 -0.12910422844718361 80 -0.1684919526365983 81 -0.20786008498339417 82 -0.24623207162942826
		 83 -0.28263352803791625 84 -0.31609230361324969 85 -0.34563832638653053 86 -0.37030319180446436
		 87 -0.38911947586175744 88 -0.4011197743268633 89 -0.40533549661986684 90 -0.40035318268067688
		 91 -0.38627038235769023 92 -0.36438110722368011 93 -0.33597811398389216 94 -0.30235469026612943
		 95 -0.2648059906935446 96 -0.22462989200470573 97 -0.18312738719376109 98 -0.1416025771948414
		 99 -0.10136234449209883 100 -0.063715806116563412 101 -0.029973643712996829 102 -0.0014473956464669574
		 103 0.020551229567608811 104 0.034710997899665502 105 0.039721783539431427 106 0.035947068057327676
		 107 0.02520430322380485 108 0.0083662978128091047 109 -0.013693444473903216 110 -0.040101049107409283
		 111 -0.069982587591595841 112 -0.10246440724136138 113 -0.13667347493055795 114 -0.17173772058692333
		 115 -0.20678635477617219 116 -0.24095012712520783 117 -0.27336148857808795 118 -0.30315462054846209
		 119 -0.32946529791019946 120 -0.3514305604320489 121 -0.36818817869235759 122 -0.37887591569234524
		 123 -0.38263060431720525 124 -0.37861571689330542 125 -0.36718722621045635 126 -0.34926787803513332
		 127 -0.32577936745451669 128 -0.29764340040240345 129 -0.26578254025725884 130 -0.23112081486954827
		 131 -0.19458408251995057 132 -0.15710017387401831 133 -0.11959884097831056 134 -0.083011553720371958
		 135 -0.048271188922635044 136 -0.016311657328846148 137 0.011932490849560471 138 0.035526450445280994
		 139 0.053535518509578828 140 0.065025425435123768 141 0.069062664680989524 142 0.06482181046672407
		 143 0.052752456770834288 144 0.033835293170691706 145 0.0090518622767342405 146 -0.020615798334162133
		 147 -0.054185519519103692 148 -0.090675397876649924 149 -0.12910422844718361 150 -0.1684919526365983
		 151 -0.20786008498339417 152 -0.24623207162942826 153 -0.28263352803791625 154 -0.31609230361324969
		 155 -0.34563832638653053 156 -0.37030319180446436;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_002_cnt_rotateY";
	rename -uid "DF00B3B9-4A7C-75EB-7E6D-BAA2854ABCB4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -0.035652184056053832 18 -0.037102138672637473
		 19 -0.037616517965256356 20 -0.037008883227159076 21 -0.035311036280229136 22 -0.032729666909289107
		 23 -0.029484597184595885 24 -0.025795380256594613 25 -0.02187031865030209 26 -0.017897900262101578
		 27 -0.014040657175785846 28 -0.010431456554894717 29 -0.0071722343570550162 30 -0.004335181902754592
		 31 -0.0019663930782184607 32 -9.1976883942893086e-05 33 0.0012733631762840527 34 0.0021152842806388575
		 35 0.0024062979355644575 36 0.0021874082097485328 37 0.0015532163064705857 38 0.00052572858552070651
		 39 -0.00088226377002894327 40 -0.0026600962575673705 41 -0.0047932348208758538 42 -0.0072583736381378961
		 43 -0.010019627901822762 44 -0.01302582167848023 45 -0.016208869934544068 46 -0.019483252758083266
		 47 -0.022746578828001966 48 -0.025881234453839656 49 -0.028757114215551362 50 -0.031235429542946162
		 51 -0.033173592622475739 52 -0.034431174864790852 53 -0.034876941785350654 54 -0.034400359808441278
		 55 -0.033056670562823562 56 -0.030988287873621685 57 -0.028348139529031424 58 -0.025291624240434588
		 59 -0.021969798743589782 60 -0.018523792825246604 61 -0.015080453288038457 62 -0.011749220125874387
		 63 -0.0086202394341324453 64 -0.0057637179085128507 65 -0.0032305233626159814 66 -0.0010540347413652733
		 67 0.00074675613205583556 68 0.0021628901424289963 69 0.003189834060999075 70 0.0038206225737375896
		 71 0.0040377512541659097 72 0.0038096096977657226 73 0.0031461524747089949 74 0.0020640542216969759
		 75 0.0005683612588919518 76 -0.0013385625207819789 77 -0.0036494681045282549 78 -0.0063460346266974174
		 79 -0.0093940649765751668 80 -0.012740064117356597 81 -0.016309199078899241 82 -0.020004638029016406
		 83 -0.023708264333257137 84 -0.027282760352885134 85 -0.030575055201347338 86 -0.033421131050137984
		 87 -0.035652184056053832 88 -0.037102138672637473 89 -0.037616517965256356 90 -0.037008883227159076
		 91 -0.035311036280229136 92 -0.032729666909289107 93 -0.029484597184595885 94 -0.025795380256594613
		 95 -0.02187031865030209 96 -0.017897900262101578 97 -0.014040657175785846 98 -0.010431456554894717
		 99 -0.0071722343570550162 100 -0.004335181902754592 101 -0.0019663930782184607 102 -9.1976883942893086e-05
		 103 0.0012733631762840527 104 0.0021152842806388575 105 0.0024062979355644575 106 0.0021874082097485328
		 107 0.0015532163064705857 108 0.00052572858552070651 109 -0.00088226377002894327
		 110 -0.0026600962575673705 111 -0.0047932348208758538 112 -0.0072583736381378961
		 113 -0.010019627901822762 114 -0.01302582167848023 115 -0.016208869934544068 116 -0.019483252758083266
		 117 -0.022746578828001966 118 -0.025881234453839656 119 -0.028757114215551362 120 -0.031235429542946162
		 121 -0.033173592622475739 122 -0.034431174864790852 123 -0.034876941785350654 124 -0.034400359808441278
		 125 -0.033056670562823562 126 -0.030988287873621685 127 -0.028348139529031424 128 -0.025291624240434588
		 129 -0.021969798743589782 130 -0.018523792825246604 131 -0.015080453288038457 132 -0.011749220125874387
		 133 -0.0086202394341324453 134 -0.0057637179085128507 135 -0.0032305233626159814
		 136 -0.0010540347413652733 137 0.00074675613205583556 138 0.0021628901424289963 139 0.003189834060999075
		 140 0.0038206225737375896 141 0.0040377512541659097 142 0.0038096096977657226 143 0.0031461524747089949
		 144 0.0020640542216969759 145 0.0005683612588919518 146 -0.0013385625207819789 147 -0.0036494681045282549
		 148 -0.0063460346266974174 149 -0.0093940649765751668 150 -0.012740064117356597 151 -0.016309199078899241
		 152 -0.020004638029016406 153 -0.023708264333257137 154 -0.027282760352885134 155 -0.030575055201347338
		 156 -0.033421131050137984;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_002_cnt_rotateZ";
	rename -uid "0167CC72-4A9F-17C2-28E6-A8B65340BB0C";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -3.2097194409781866 18 -3.3089927886021582
		 19 -3.3438725414436985 20 -3.302650489422617 21 -3.1861531035598145 22 -3.0051332741529682
		 23 -2.770343739549888 24 -2.492537300553535 25 -2.1824669813730946 26 -1.8508861333003133
		 27 -1.5085484832341935 28 -1.1662081337522738 29 -0.83461952459055722 30 -0.52453736709824683
		 31 -0.2467165634022869 32 -0.011912120600816683 33 0.16912093273355322 34 0.28562762512294732
		 35 0.3268531194903378 36 0.29579736656359223 37 0.20740799569944821 38 0.068851928252999586
		 39 -0.11270383007826446 40 -0.33009222877974903 41 -0.57614621115040476 42 -0.84369875445217291
		 43 -1.1255829116658405 44 -1.4146318530870163 45 -1.703678904624633 46 -1.985557578770982
		 47 -2.2531015937992436 48 -2.4991448767968003 49 -2.716521546652801 50 -2.8980658740648675
		 51 -3.0366122169988641 52 -3.124994931817882 53 -3.1560482624866832 54 -3.1228430181729956
		 55 -3.0283356034476414 56 -2.8801883135384299 57 -2.6860633166629877 58 -2.4536227815101546
		 59 -2.1905289791488061 60 -1.9044443564163025 61 -1.6030315805161406 62 -1.2939535567335816
		 63 -0.98487342285541091 64 -0.68345452504382642 65 -0.39736038054078021 66 -0.13425463264764673
		 67 0.098198997080327671 68 0.29233675557929412 69 0.44049490179804113 70 0.53500974710124238
		 71 0.56821769551185719 72 0.53333490341854106 73 0.43405320417519266 74 0.27842271985280298
		 75 0.074493676096707581 76 -0.1696836415885046 77 -0.44605893219515957 78 -0.74658192763509434
		 79 -1.0632024453484727 80 -1.3878704425859785 81 -1.7125360679012089 82 -2.0291497041263646
		 83 -2.3296619965211072 84 -2.6060238598696475 85 -2.8501864590341577 86 -3.0541011588231761
		 87 -3.2097194409781866 88 -3.3089927886021582 89 -3.3438725414436985 90 -3.302650489422617
		 91 -3.1861531035598145 92 -3.0051332741529682 93 -2.770343739549888 94 -2.492537300553535
		 95 -2.1824669813730946 96 -1.8508861333003133 97 -1.5085484832341935 98 -1.1662081337522738
		 99 -0.83461952459055722 100 -0.52453736709824683 101 -0.2467165634022869 102 -0.011912120600816683
		 103 0.16912093273355322 104 0.28562762512294732 105 0.3268531194903378 106 0.29579736656359223
		 107 0.20740799569944821 108 0.068851928252999586 109 -0.11270383007826446 110 -0.33009222877974903
		 111 -0.57614621115040476 112 -0.84369875445217291 113 -1.1255829116658405 114 -1.4146318530870163
		 115 -1.703678904624633 116 -1.985557578770982 117 -2.2531015937992436 118 -2.4991448767968003
		 119 -2.716521546652801 120 -2.8980658740648675 121 -3.0366122169988641 122 -3.124994931817882
		 123 -3.1560482624866832 124 -3.1228430181729956 125 -3.0283356034476414 126 -2.8801883135384299
		 127 -2.6860633166629877 128 -2.4536227815101546 129 -2.1905289791488061 130 -1.9044443564163025
		 131 -1.6030315805161406 132 -1.2939535567335816 133 -0.98487342285541091 134 -0.68345452504382642
		 135 -0.39736038054078021 136 -0.13425463264764673 137 0.098198997080327671 138 0.29233675557929412
		 139 0.44049490179804113 140 0.53500974710124238 141 0.56821769551185719 142 0.53333490341854106
		 143 0.43405320417519266 144 0.27842271985280298 145 0.074493676096707581 146 -0.1696836415885046
		 147 -0.44605893219515957 148 -0.74658192763509434 149 -1.0632024453484727 150 -1.3878704425859785
		 151 -1.7125360679012089 152 -2.0291497041263646 153 -2.3296619965211072 154 -2.6060238598696475
		 155 -2.8501864590341577 156 -3.0541011588231761;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_001_cnt_translateX";
	rename -uid "3BC338DD-402C-248F-C242-0D8C67B1C7BD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -0.29566612100782663 18 -0.29606473995073657
		 19 -0.29724447357527595 20 -0.29922703528988848 21 -0.30204588272187038 22 -0.30569655681838981
		 23 -0.31010161321459861 24 -0.31509017051376986 25 -0.32039209999753382 26 -0.32564686231495443
		 27 -0.33042697187481451 28 -0.33427605493638168 29 -0.33676147614836793 30 -0.33868123620055712
		 31 -0.34099193984067711 32 -0.34358882766929355 33 -0.3463994139707296 34 -0.34947419565959592
		 35 -0.35296045334618498 36 -0.35694098867216439 37 -0.36148358761553823 38 -0.36663302174565615
		 39 -0.37240469174965085 40 -0.37874842746410309 41 -0.38556448950809852 42 -0.39273917316354812
		 43 -0.40012016262568295 44 -0.40751923407010793 45 -0.41471663145539139 46 -0.42146708453694259
		 47 -0.4275074413561839 48 -0.43256589461074668 49 -0.43637279307969834 50 -0.43867304553508291
		 51 -0.43924014466324479 52 -0.4378395468707339 53 -0.43456828241582457 54 -0.42975966215195172
		 55 -0.42377112898144276 56 -0.41696409032680037 57 -0.40968685415288064 58 -0.40226064791147564
		 59 -0.39496873184572223 60 -0.3880486405590915 61 -0.38173386775044094 62 -0.37618860189351722
		 63 -0.37147498130242695 64 -0.36761504466153383 65 -0.36459989068443122 66 -0.36240183912110524
		 67 -0.36098958964903716 68 -0.36034636435401524 69 -0.36049100953584912 70 -0.36015147535643166
		 71 -0.35816411927451952 72 -0.35478719223074506 73 -0.35030963561170836 74 -0.3450319302434508
		 75 -0.33924980075438782 76 -0.3332407582098682 77 -0.32725347712718644 78 -0.3215000121725069
		 79 -0.31615086438257833 80 -0.31133290698824112 81 -0.30713017785501506 82 -0.30358754067739824
		 83 -0.30071721201186108 84 -0.29850814762238542 85 -0.29693828074148598 86 -0.29598960750614367
		 87 -0.29566612100782663 88 -0.29606473995073657 89 -0.29724447357527595 90 -0.29922703528988848
		 91 -0.30204588272187038 92 -0.30569655681838981 93 -0.31010161321459861 94 -0.31509017051376986
		 95 -0.32039209999753382 96 -0.32564686231495443 97 -0.33042697187481451 98 -0.33427605493638168
		 99 -0.33676147614836793 100 -0.33868123620055712 101 -0.34099193984067711 102 -0.34358882766929355
		 103 -0.3463994139707296 104 -0.34947419565959592 105 -0.35296045334618498 106 -0.35694098867216439
		 107 -0.36148358761553823 108 -0.36663302174565615 109 -0.37240469174965085 110 -0.37874842746410309
		 111 -0.38556448950809852 112 -0.39273917316354812 113 -0.40012016262568295 114 -0.40751923407010793
		 115 -0.41471663145539139 116 -0.42146708453694259 117 -0.4275074413561839 118 -0.43256589461074668
		 119 -0.43637279307969834 120 -0.43867304553508291 121 -0.43924014466324479 122 -0.4378395468707339
		 123 -0.43456828241582457 124 -0.42975966215195172 125 -0.42377112898144276 126 -0.41696409032680037
		 127 -0.40968685415288064 128 -0.40226064791147564 129 -0.39496873184572223 130 -0.3880486405590915
		 131 -0.38173386775044094 132 -0.37618860189351722 133 -0.37147498130242695 134 -0.36761504466153383
		 135 -0.36459989068443122 136 -0.36240183912110524 137 -0.36098958964903716 138 -0.36034636435401524
		 139 -0.36049100953584912 140 -0.36015147535643166 141 -0.35816411927451952 142 -0.35478719223074506
		 143 -0.35030963561170836 144 -0.3450319302434508 145 -0.33924980075438782 146 -0.3332407582098682
		 147 -0.32725347712718644 148 -0.3215000121725069 149 -0.31615086438257833 150 -0.31133290698824112
		 151 -0.30713017785501506 152 -0.30358754067739824 153 -0.30071721201186108 154 -0.29850814762238542
		 155 -0.29693828074148598 156 -0.29598960750614367;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_001_cnt_translateY";
	rename -uid "93394289-4CF9-8FA1-B551-C187E2F454F2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -0.72391023816176414 18 -0.72318991104785368
		 19 -0.72101864203789034 20 -0.71730697470695759 21 -0.71191651856634053 22 -0.70471486918958703
		 23 -0.69562146578064699 24 -0.68464431244370871 25 -0.67190777503226684 26 -0.65767189340853349
		 27 -0.64234380664306912 28 -0.62648195167737697 29 -0.61079363413655585 30 -0.59658785407258108
		 31 -0.5847419253043995 32 -0.57545434799591533 33 -0.56897475457277835 34 -0.56502000574840849
		 35 -0.56286056317129862 36 -0.5621760401845961 37 -0.56272171656016212 38 -0.5643284978760903
		 39 -0.56690050828314043 40 -0.56992945508045523 41 -0.57277865077465151 42 -0.57523898485071356
		 43 -0.57713766398609323 44 -0.57833768899568838 45 -0.57873634265561691 46 -0.57826261572814985
		 47 -0.5768735210031366 48 -0.57454927257835209 49 -0.57128733928431075 50 -0.56709541680220354
		 51 -0.56198340274694658 52 -0.55599405601495278 53 -0.54936673322791307 54 -0.54236398378048989
		 55 -0.53520217756046407 56 -0.5280581931956192 57 -0.521076038430067 58 -0.5143733693874708
		 59 -0.50804794295740408 60 -0.50218408608098741 61 -0.49756584229425016 62 -0.49470797112576292
		 63 -0.49333399226806307 64 -0.49325030163652173 65 -0.49436389424433003 66 -0.4966955741673047
		 67 -0.50038865258702003 68 -0.5057130885654999 69 -0.51306496698754245 70 -0.52359359105561509
		 71 -0.53747771226019836 72 -0.55378434398734555 73 -0.57164286395403963 74 -0.59026204792377257
		 75 -0.60894378053559706 76 -0.62709334347451318 77 -0.64422607795781062 78 -0.65997015821257321
		 79 -0.67406519091989026 80 -0.68635636648997433 81 -0.6967839243190781 82 -0.70536774818741321
		 83 -0.71218697234287731 84 -0.71735454762234951 85 -0.72098678670270999 86 -0.72316797843600966
		 87 -0.72391023816176414 88 -0.72318991104785368 89 -0.72101864203789034 90 -0.71730697470695759
		 91 -0.71191651856634053 92 -0.70471486918958703 93 -0.69562146578064699 94 -0.68464431244370871
		 95 -0.67190777503226684 96 -0.65767189340853349 97 -0.64234380664306912 98 -0.62648195167737697
		 99 -0.61079363413655585 100 -0.59658785407258108 101 -0.5847419253043995 102 -0.57545434799591533
		 103 -0.56897475457277835 104 -0.56502000574840849 105 -0.56286056317129862 106 -0.5621760401845961
		 107 -0.56272171656016212 108 -0.5643284978760903 109 -0.56690050828314043 110 -0.56992945508045523
		 111 -0.57277865077465151 112 -0.57523898485071356 113 -0.57713766398609323 114 -0.57833768899568838
		 115 -0.57873634265561691 116 -0.57826261572814985 117 -0.5768735210031366 118 -0.57454927257835209
		 119 -0.57128733928431075 120 -0.56709541680220354 121 -0.56198340274694658 122 -0.55599405601495278
		 123 -0.54936673322791307 124 -0.54236398378048989 125 -0.53520217756046407 126 -0.5280581931956192
		 127 -0.521076038430067 128 -0.5143733693874708 129 -0.50804794295740408 130 -0.50218408608098741
		 131 -0.49756584229425016 132 -0.49470797112576292 133 -0.49333399226806307 134 -0.49325030163652173
		 135 -0.49436389424433003 136 -0.4966955741673047 137 -0.50038865258702003 138 -0.5057130885654999
		 139 -0.51306496698754245 140 -0.52359359105561509 141 -0.53747771226019836 142 -0.55378434398734555
		 143 -0.57164286395403963 144 -0.59026204792377257 145 -0.60894378053559706 146 -0.62709334347451318
		 147 -0.64422607795781062 148 -0.65997015821257321 149 -0.67406519091989026 150 -0.68635636648997433
		 151 -0.6967839243190781 152 -0.70536774818741321 153 -0.71218697234287731 154 -0.71735454762234951
		 155 -0.72098678670270999 156 -0.72316797843600966;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_tail_jnt_001_cnt_translateZ";
	rename -uid "AC0719EE-4D58-9AC4-14C2-38A39E074D39";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -0.069771936151315161 18 -0.059961490552202235
		 19 -0.032765619847041738 20 0.0084530860245027872 21 0.060320782599830869 22 0.11945461146138381
		 23 0.18246642608281949 24 0.24596806465030885 25 0.30657759906564941 26 0.36092561662328304
		 27 0.40566047632448765 28 0.43745161582826786 29 0.4529903517193179 30 0.46502072774235498
		 31 0.48773931993737285 32 0.52004194092968514 33 0.56083685605615419 34 0.60904140886031788
		 35 0.66355543953022711 36 0.72325683864456081 37 0.78700818880670043 38 0.85365884470665199
		 39 0.92204731697978848 40 0.9912923448119304 41 1.0605461802763156 42 1.1286907278249088
		 43 1.1946085969775773 44 1.2571841662090972 45 1.3153043051022166 46 1.3678586336529617
		 47 1.4137392198294667 48 1.451839648366982 49 1.4810534356047111 50 1.5002718170778255
		 51 1.5083809965793058 52 1.5040142147037989 53 1.4875210257962097 54 1.4603539255074407
		 55 1.4239654724589264 56 1.379812065554074 57 1.3293568777226881 58 1.2740718249154406
		 59 1.2154385417276168 60 1.1549484105469667 61 1.0937644666959521 62 1.0331249600860886
		 63 0.97463652620773711 64 0.91989974560822207 65 0.87050787765768667 66 0.82804435567519208
		 67 0.79407926328814971 68 0.77016505146154779 69 0.7578317570082016 70 0.74533589037556669
		 71 0.72084728727935277 72 0.68576313337680062 73 0.64150934960084649 74 0.58954582120324961
		 75 0.53136885627366193 76 0.4685110879989427 77 0.40253915644938948 78 0.33504960493448682
		 79 0.26766349766932085 80 0.20202030339023391 81 0.139771587324515 82 0.08257500586567712
		 83 0.032088999945420027 84 -0.010031568476698838 85 -0.04213879824976563 86 -0.062594369923643201
		 87 -0.069771936151315161 88 -0.059961490552202235 89 -0.032765619847041738 90 0.0084530860245027872
		 91 0.060320782599830869 92 0.11945461146138381 93 0.18246642608281949 94 0.24596806465030885
		 95 0.30657759906564941 96 0.36092561662328304 97 0.40566047632448765 98 0.43745161582826786
		 99 0.4529903517193179 100 0.46502072774235498 101 0.48773931993737285 102 0.52004194092968514
		 103 0.56083685605615419 104 0.60904140886031788 105 0.66355543953022711 106 0.72325683864456081
		 107 0.78700818880670043 108 0.85365884470665199 109 0.92204731697978848 110 0.9912923448119304
		 111 1.0605461802763156 112 1.1286907278249088 113 1.1946085969775773 114 1.2571841662090972
		 115 1.3153043051022166 116 1.3678586336529617 117 1.4137392198294667 118 1.451839648366982
		 119 1.4810534356047111 120 1.5002718170778255 121 1.5083809965793058 122 1.5040142147037989
		 123 1.4875210257962097 124 1.4603539255074407 125 1.4239654724589264 126 1.379812065554074
		 127 1.3293568777226881 128 1.2740718249154406 129 1.2154385417276168 130 1.1549484105469667
		 131 1.0937644666959521 132 1.0331249600860886 133 0.97463652620773711 134 0.91989974560822207
		 135 0.87050787765768667 136 0.82804435567519208 137 0.79407926328814971 138 0.77016505146154779
		 139 0.7578317570082016 140 0.74533589037556669 141 0.72084728727935277 142 0.68576313337680062
		 143 0.64150934960084649 144 0.58954582120324961 145 0.53136885627366193 146 0.4685110879989427
		 147 0.40253915644938948 148 0.33504960493448682 149 0.26766349766932085 150 0.20202030339023391
		 151 0.139771587324515 152 0.08257500586567712 153 0.032088999945420027 154 -0.010031568476698838
		 155 -0.04213879824976563 156 -0.062594369923643201;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_001_cnt_rotateX";
	rename -uid "B4F2CFDA-4B64-E3A7-C405-3AB4EF23979F";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 7.7169224918064145 18 7.647101138102629
		 19 7.4491516363703818 20 7.1402054366959282 21 6.7373570598326538 22 6.2578281018136659
		 23 5.7191035143412359 24 5.1390358153264994 25 4.5359086103061301 26 3.9284488445335319
		 27 3.335777699516258 28 2.7772929531293968 29 2.2724806682855601 30 1.7765461111395864
		 31 1.237460310974192 32 0.66383385143015639 33 0.064397853016367398 34 -0.5518707314885074
		 35 -1.1756306545880117 36 -1.7972957457639602 37 -2.4070694702120914 38 -2.9949853716021888
		 39 -3.5509442402250282 40 -4.0894444971598221 41 -4.6278770591765355 42 -5.1604381091193225
		 43 -5.6813700068244 44 -6.1849551157377789 45 -6.6655050180349242 46 -7.1173458371278606
		 47 -7.53480031286208 48 -7.9121671195383971 49 -8.2436976687882026 50 -8.5235702909666333
		 51 -8.7458612384564702 52 -8.903490645582453 53 -8.9961790120946183 54 -9.0281136578671699
		 55 -9.0035320466988953 56 -8.9267631703671082 57 -8.8022585059574965 58 -8.6346143458172975
		 59 -8.4285865419229022 60 -8.1890979872975969 61 -7.8885824210472197 62 -7.5055532382386714
		 63 -7.0541378831131087 64 -6.5483796461458281 65 -6.0021918320653143 66 -5.4292994019199021
		 67 -4.8431671930808227 68 -4.2569152796051117 69 -3.6832234220873232 70 -3.079713601996485
		 71 -2.4037251070511569 72 -1.6674735717089568 73 -0.88357958362037614 74 -0.064983774619877352
		 75 0.77514087085824368 76 1.6234751279549424 77 2.4666335543213971 78 3.2912343079000239
		 79 4.0839554887594991 80 4.8315758802844817 81 5.5209992279853273 82 6.1392623857002544
		 83 6.6735286145697481 84 7.1110679218754802 85 7.4392265183179607 86 7.6453872676734198
		 87 7.7169224918064145 88 7.647101138102629 89 7.4491516363703818 90 7.1402054366959282
		 91 6.7373570598326538 92 6.2578281018136659 93 5.7191035143412359 94 5.1390358153264994
		 95 4.5359086103061301 96 3.9284488445335319 97 3.335777699516258 98 2.7772929531293968
		 99 2.2724806682855601 100 1.7765461111395864 101 1.237460310974192 102 0.66383385143015639
		 103 0.064397853016367398 104 -0.5518707314885074 105 -1.1756306545880117 106 -1.7972957457639602
		 107 -2.4070694702120914 108 -2.9949853716021888 109 -3.5509442402250282 110 -4.0894444971598221
		 111 -4.6278770591765355 112 -5.1604381091193225 113 -5.6813700068244 114 -6.1849551157377789
		 115 -6.6655050180349242 116 -7.1173458371278606 117 -7.53480031286208 118 -7.9121671195383971
		 119 -8.2436976687882026 120 -8.5235702909666333 121 -8.7458612384564702 122 -8.903490645582453
		 123 -8.9961790120946183 124 -9.0281136578671699 125 -9.0035320466988953 126 -8.9267631703671082
		 127 -8.8022585059574965 128 -8.6346143458172975 129 -8.4285865419229022 130 -8.1890979872975969
		 131 -7.8885824210472197 132 -7.5055532382386714 133 -7.0541378831131087 134 -6.5483796461458281
		 135 -6.0021918320653143 136 -5.4292994019199021 137 -4.8431671930808227 138 -4.2569152796051117
		 139 -3.6832234220873232 140 -3.079713601996485 141 -2.4037251070511569 142 -1.6674735717089568
		 143 -0.88357958362037614 144 -0.064983774619877352 145 0.77514087085824368 146 1.6234751279549424
		 147 2.4666335543213971 148 3.2912343079000239 149 4.0839554887594991 150 4.8315758802844817
		 151 5.5209992279853273 152 6.1392623857002544 153 6.6735286145697481 154 7.1110679218754802
		 155 7.4392265183179607 156 7.6453872676734198;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_001_cnt_rotateY";
	rename -uid "DCA361D6-40E6-C561-3AB0-2EA7B82D67E9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -4.934265381929948 18 -4.9898671499131364
		 19 -5.141336213681674 20 -5.3652713628659257 21 -5.6379405062233641 22 -5.9355052060950486
		 23 -6.2342231041326546 24 -6.5106302749205733 25 -6.7417009682833218 26 -6.904978672453006
		 27 -6.9786700742961303 28 -6.9416924651083214 29 -6.7736655838713142 30 -6.5713806409555939
		 31 -6.4418962527143062 32 -6.3811568330809081 33 -6.3856526690062498 34 -6.4523519942312832
		 35 -6.578011780556877 36 -6.7589455893396604 37 -6.9911200298921115 38 -7.2701310406379651
		 39 -7.5911958915724611 40 -7.9358351096649793 41 -8.2840949400539952 42 -8.6285318149242602
		 43 -8.961481768449131 44 -9.2750706074901856 45 -9.5612305868594536 46 -9.8117229636532919
		 47 -10.018165884887173 48 -10.172067184955502 49 -10.264861835962122 50 -10.287954006799724
		 51 -10.232763948554908 52 -10.088990239786551 53 -9.8600750296260671 54 -9.5582651999950681
		 55 -9.1959910652660799 56 -8.7858191242633978 57 -8.3404090655117571 58 -7.8724742645724861
		 59 -7.3947452337118333 60 -6.9199356877257943 61 -6.4786672983652815 62 -6.0988758206381437
		 63 -5.7889767196045714 64 -5.5570281718436512 65 -5.4106878278844617 66 -5.3571808014876643
		 67 -5.4032821852831194 68 -5.555317490419668 69 -5.8191841358035532 70 -6.1045487015702404
		 71 -6.3197268602438514 72 -6.4684385722536568 73 -6.5547493811742727 74 -6.5832265350729022
		 75 -6.5590541111645537 76 -6.4881072348586217 77 -6.3769866874938916 78 -6.2330163294176462
		 79 -6.064206711656321 80 -5.8791889013920979 81 -5.6871228152075455 82 -5.4975841795658162
		 83 -5.3204335910817901 84 -5.1656700289299238 85 -5.0432696005601159 86 -4.9630083201104496
		 87 -4.934265381929948 88 -4.9898671499131364 89 -5.141336213681674 90 -5.3652713628659257
		 91 -5.6379405062233641 92 -5.9355052060950486 93 -6.2342231041326546 94 -6.5106302749205733
		 95 -6.7417009682833218 96 -6.904978672453006 97 -6.9786700742961303 98 -6.9416924651083214
		 99 -6.7736655838713142 100 -6.5713806409555939 101 -6.4418962527143062 102 -6.3811568330809081
		 103 -6.3856526690062498 104 -6.4523519942312832 105 -6.578011780556877 106 -6.7589455893396604
		 107 -6.9911200298921115 108 -7.2701310406379651 109 -7.5911958915724611 110 -7.9358351096649793
		 111 -8.2840949400539952 112 -8.6285318149242602 113 -8.961481768449131 114 -9.2750706074901856
		 115 -9.5612305868594536 116 -9.8117229636532919 117 -10.018165884887173 118 -10.172067184955502
		 119 -10.264861835962122 120 -10.287954006799724 121 -10.232763948554908 122 -10.088990239786551
		 123 -9.8600750296260671 124 -9.5582651999950681 125 -9.1959910652660799 126 -8.7858191242633978
		 127 -8.3404090655117571 128 -7.8724742645724861 129 -7.3947452337118333 130 -6.9199356877257943
		 131 -6.4786672983652815 132 -6.0988758206381437 133 -5.7889767196045714 134 -5.5570281718436512
		 135 -5.4106878278844617 136 -5.3571808014876643 137 -5.4032821852831194 138 -5.555317490419668
		 139 -5.8191841358035532 140 -6.1045487015702404 141 -6.3197268602438514 142 -6.4684385722536568
		 143 -6.5547493811742727 144 -6.5832265350729022 145 -6.5590541111645537 146 -6.4881072348586217
		 147 -6.3769866874938916 148 -6.2330163294176462 149 -6.064206711656321 150 -5.8791889013920979
		 151 -5.6871228152075455 152 -5.4975841795658162 153 -5.3204335910817901 154 -5.1656700289299238
		 155 -5.0432696005601159 156 -4.9630083201104496;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_tail_jnt_001_cnt_rotateZ";
	rename -uid "3C5D4A14-4CEE-9C6F-946C-78A24D63B463";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 21.250796903791365 18 21.289488126552268
		 19 21.400074819818954 20 21.57494289930397 21 21.806800912379565 22 22.088141126275914
		 23 22.410805642688796 24 22.765661075534982 25 23.142385332080757 26 23.529369572542794
		 27 23.913737147179717 28 24.281478986407986 29 24.617701540386914 30 24.909818995612994
		 31 25.149184096656345 32 25.326285279942432 33 25.431444781036717 34 25.468579115593986
		 35 25.453293012946592 36 25.392942404876315 37 25.294410404902159 38 25.16406107114857
		 39 25.00770895900008 40 24.834019262972802 41 24.652160477661525 42 24.468029896445444
		 43 24.287190236034309 44 24.114877356612844 45 23.956018262029559 46 23.815259938055892
		 47 23.697009566152943 48 23.605486604994034 49 23.544787156237565 50 23.518960921929395
		 51 23.532100921213345 52 23.587821596549581 53 23.682783903275027 54 23.811117815437647
		 55 23.967372317669046 56 24.146416379474186 57 24.343342596178548 58 24.553374725730034
		 59 24.771780274758278 60 24.993789175902432 61 25.210839864629889 62 25.415736673043497
		 63 25.6050423771336 64 25.774415114388795 65 25.918583029794267 66 26.031358367695677
		 67 26.105687870403379 68 26.133735624656833 69 26.106993874488655 70 26.006583995853539
		 71 25.827775794613217 72 25.582956031599497 73 25.284490195491362 74 24.944543513002479
		 75 24.574916948908431 76 24.186902921558499 77 23.791166198801704 78 23.397655615920272
		 79 23.015551927290094 80 22.653256371028274 81 22.318423520585021 82 22.018040847896113
		 83 21.758556234227164 84 21.54605350174435 85 21.3864749179535 86 21.285888516001023
		 87 21.250796903791365 88 21.289488126552268 89 21.400074819818954 90 21.57494289930397
		 91 21.806800912379565 92 22.088141126275914 93 22.410805642688796 94 22.765661075534982
		 95 23.142385332080757 96 23.529369572542794 97 23.913737147179717 98 24.281478986407986
		 99 24.617701540386914 100 24.909818995612994 101 25.149184096656345 102 25.326285279942432
		 103 25.431444781036717 104 25.468579115593986 105 25.453293012946592 106 25.392942404876315
		 107 25.294410404902159 108 25.16406107114857 109 25.00770895900008 110 24.834019262972802
		 111 24.652160477661525 112 24.468029896445444 113 24.287190236034309 114 24.114877356612844
		 115 23.956018262029559 116 23.815259938055892 117 23.697009566152943 118 23.605486604994034
		 119 23.544787156237565 120 23.518960921929395 121 23.532100921213345 122 23.587821596549581
		 123 23.682783903275027 124 23.811117815437647 125 23.967372317669046 126 24.146416379474186
		 127 24.343342596178548 128 24.553374725730034 129 24.771780274758278 130 24.993789175902432
		 131 25.210839864629889 132 25.415736673043497 133 25.6050423771336 134 25.774415114388795
		 135 25.918583029794267 136 26.031358367695677 137 26.105687870403379 138 26.133735624656833
		 139 26.106993874488655 140 26.006583995853539 141 25.827775794613217 142 25.582956031599497
		 143 25.284490195491362 144 24.944543513002479 145 24.574916948908431 146 24.186902921558499
		 147 23.791166198801704 148 23.397655615920272 149 23.015551927290094 150 22.653256371028274
		 151 22.318423520585021 152 22.018040847896113 153 21.758556234227164 154 21.54605350174435
		 155 21.3864749179535 156 21.285888516001023;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTU -n "cn_tail_jnt_001_cnt_worldspace";
	rename -uid "A78A6324-42A8-982A-CC0B-1BA9B15511B0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 1 18 1 19 1 20 1 21 1 22 1 23 1 24 1
		 25 1 26 1 27 1 28 1 29 1 30 1 31 1 32 1 33 1 34 1 35 1 36 1 37 1 38 1 39 1 40 1 41 1
		 42 1 43 1 44 1 45 1 46 1 47 1 48 1 49 1 50 1 51 1 52 1 53 1 54 1 55 1 56 1 57 1 58 1
		 59 1 60 1 61 1 62 1 63 1 64 1 65 1 66 1 67 1 68 1 69 1 70 1 71 1 72 1 73 1 74 1 75 1
		 76 1 77 1 78 1 79 1 80 1 81 1 82 1 83 1 84 1 85 1 86 1 87 1 88 1 89 1 90 1 91 1 92 1
		 93 1 94 1 95 1 96 1 97 1 98 1 99 1 100 1 101 1 102 1 103 1 104 1 105 1 106 1 107 1
		 108 1 109 1 110 1 111 1 112 1 113 1 114 1 115 1 116 1 117 1 118 1 119 1 120 1 121 1
		 122 1 123 1 124 1 125 1 126 1 127 1 128 1 129 1 130 1 131 1 132 1 133 1 134 1 135 1
		 136 1 137 1 138 1 139 1 140 1 141 1 142 1 143 1 144 1 145 1 146 1 147 1 148 1 149 1
		 150 1 151 1 152 1 153 1 154 1 155 1 156 1;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_bk_foot_001_cnt_translateX";
	rename -uid "F65E643D-4555-1F34-3CEC-F18ED28580D9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0 79 0 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0 88 0 89 0 90 0 91 0 92 0
		 93 0 94 0 95 0 96 0 97 0 98 0 99 0 100 0 101 0 102 0 103 0 104 0 105 0 106 0 107 0
		 108 0 109 0 110 0 111 0 112 0 113 0 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0
		 122 0 123 0 124 0 125 0 126 0 127 0 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0
		 136 0 137 0 138 0 139 0 140 0 141 0 142 0 143 0 144 0 145 0 146 0 147 0 148 0 149 0
		 150 0 151 0 152 0 153 0 154 0 155 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_bk_foot_001_cnt_translateY";
	rename -uid "8B243FD6-4E70-AC4A-2FA5-F7B4D40EB8A2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 5.4159838282078656 18 5.9432594780681089
		 19 6.3901035790231671 20 6.74921629317687 21 7.0146689375024298 22 7.1803131386829744
		 23 7.2379602876699156 24 7.2432477530519517 25 7.2579707708939001 26 7.2795978110238462
		 27 7.3047114193059404 28 7.3292519224740271 29 7.3487440929967338 30 7.3585123902893326
		 31 7.3538879524095631 32 7.3304081392014293 33 7.2840073095362694 34 7.2111957955350388
		 35 7.1092228188807791 36 6.9762184404291769 37 6.8113095623405098 38 6.6147054848126903
		 39 6.3877494862457755 40 6.1329342361512413 41 5.853880423206979 42 5.5552796258263708
		 43 5.2428040025562783 44 4.9229866755468858 45 4.6030775843418592 46 4.2908799934245483
		 47 3.9945726790914957 48 3.7225220766885538 49 3.4830873595653475 50 3.2844196100171441
		 51 3.1342540331070605 52 3.0396916974333212 53 3.0069647429677957 54 2.8447097709087004
		 55 2.3983599399974835 56 1.7285284220579218 57 0.9541042888734399 58 0.28545741331151397
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0.10573012074848975 79 0.40151299384781325 80 0.85224638635409455 81 1.4207493033180505
		 82 2.069862988124564 83 2.764212940623711 84 3.471610467958298 85 4.1640555906399808
		 86 4.8183046097531683 87 5.4159838282078656 88 5.9432594780681089 89 6.3901035790231671
		 90 6.74921629317687 91 7.0146689375024298 92 7.1803131386829744 93 7.2379602876699156
		 94 7.2432477530519517 95 7.2579707708939001 96 7.2795978110238462 97 7.3047114193059404
		 98 7.3292519224740271 99 7.3487440929967338 100 7.3585123902893326 101 7.3538879524095631
		 102 7.3304081392014293 103 7.2840073095362694 104 7.2111957955350388 105 7.1092228188807791
		 106 6.9762184404291769 107 6.8113095623405098 108 6.6147054848126903 109 6.3877494862457755
		 110 6.1329342361512413 111 5.853880423206979 112 5.5552796258263708 113 5.2428040025562783
		 114 4.9229866755468858 115 4.6030775843418592 116 4.2908799934245483 117 3.9945726790914957
		 118 3.7225220766885538 119 3.4830873595653475 120 3.2844196100171441 121 3.1342540331070605
		 122 3.0396916974333212 123 3.0069647429677957 124 2.8447097709087004 125 2.3983599399974835
		 126 1.7285284220579218 127 0.9541042888734399 128 0.28545741331151397 129 0 130 0
		 131 0 132 0 133 0 134 0 135 0 136 0 137 0 138 0 139 0 140 0 141 0 142 0 143 0 144 0
		 145 0 146 0 147 0 148 0.10573012074848975 149 0.40151299384781325 150 0.85224638635409455
		 151 1.4207493033180505 152 2.069862988124564 153 2.764212940623711 154 3.471610467958298
		 155 4.1640555906399808 156 4.8183046097531683;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_bk_foot_001_cnt_translateZ";
	rename -uid "535E7F8A-4AA3-D2EE-473C-7993D47E0BF2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 16.620572032163906 18 16.470358113770402
		 19 16.322944905886771 20 16.190334575656891 21 16.083060819071235 22 16.010973957913265
		 23 15.984391532536421 24 15.86464290750871 25 15.516611202205368 26 14.957503657792401
		 27 14.204887097694893 28 13.276494406475123 29 12.190041946927451 30 10.963067943990453
		 31 9.6127995374907442 32 8.1560540616335828 33 6.6091780286869692 34 4.9880252169494952
		 35 3.3079731839746387 36 1.5839754878835208 37 -0.16935501935085995 38 -1.9376381636273976
		 39 -3.7066014016106834 40 -5.4619464356263592 41 -7.1892166527925809 42 -8.8736745137633761
		 43 -10.500197465078955 44 -12.053199925374997 45 -13.516587474602062 46 -14.873747654623145
		 47 -16.107579875759384 48 -17.200564917045668 49 -18.13487248547608 50 -18.892503301275866
		 51 -19.455460192738798 52 -19.805940648520519 53 -19.92654106041438 54 -20.23597483546888
		 55 -20.982783609964372 56 -21.887695675435197 57 -22.63748465257973 58 -22.888757202012428
		 59 -22.290749371088282 60 -20.89022680679912 61 -19.021076406271789 62 -16.7688750556907
		 63 -14.219199641240209 64 -11.457627049104701 65 -8.5697341654685175 66 -5.6410978765161133
		 67 -2.7572950684318016 68 -0.0039026273999525074 69 2.533502560394993 70 4.7693436087686933
		 71 6.7332297996986341 72 8.5132024163011124 73 10.111332574100146 74 11.529691388619682
		 75 12.770349975383709 76 13.835379449916218 77 14.726850927741154 78 15.447010925501687
		 79 16.006822490920655 80 16.421251186796496 81 16.705506714327072 82 16.875516249160668
		 83 16.948130493918825 84 16.941014494626017 85 16.872251601126678 86 16.759748802704834
		 87 16.620572032163906 88 16.470358113770402 89 16.322944905886771 90 16.190334575656891
		 91 16.083060819071235 92 16.010973957913265 93 15.984391532536421 94 15.86464290750871
		 95 15.516611202205368 96 14.957503657792401 97 14.204887097694893 98 13.276494406475123
		 99 12.190041946927451 100 10.963067943990453 101 9.6127995374907442 102 8.1560540616335828
		 103 6.6091780286869692 104 4.9880252169494952 105 3.3079731839746387 106 1.5839754878835208
		 107 -0.16935501935085995 108 -1.9376381636273976 109 -3.7066014016106834 110 -5.4619464356263592
		 111 -7.1892166527925809 112 -8.8736745137633761 113 -10.500197465078955 114 -12.053199925374997
		 115 -13.516587474602062 116 -14.873747654623145 117 -16.107579875759384 118 -17.200564917045668
		 119 -18.13487248547608 120 -18.892503301275866 121 -19.455460192738798 122 -19.805940648520519
		 123 -19.92654106041438 124 -20.23597483546888 125 -20.982783609964372 126 -21.887695675435197
		 127 -22.63748465257973 128 -22.888757202012428 129 -22.290749371088282 130 -20.89022680679912
		 131 -19.021076406271789 132 -16.7688750556907 133 -14.219199641240209 134 -11.457627049104701
		 135 -8.5697341654685175 136 -5.6410978765161133 137 -2.7572950684318016 138 -0.0039026273999525074
		 139 2.533502560394993 140 4.7693436087686933 141 6.7332297996986341 142 8.5132024163011124
		 143 10.111332574100146 144 11.529691388619682 145 12.770349975383709 146 13.835379449916218
		 147 14.726850927741154 148 15.447010925501687 149 16.006822490920655 150 16.421251186796496
		 151 16.705506714327072 152 16.875516249160668 153 16.948130493918825 154 16.941014494626017
		 155 16.872251601126678 156 16.759748802704834;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_bk_foot_001_cnt_rotateX";
	rename -uid "A59B8680-4E7A-10C1-871D-2393A2EE8FA4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 -23.197100364045763 18 -26.063599194745706
		 19 -28.631849592193639 20 -30.802435411972194 21 -32.475940509664078 22 -33.552948740851932
		 23 -33.934043961118384 24 -33.751205751491483 25 -33.219312778031359 26 -32.363297523868987
		 27 -31.208092472135302 28 -29.778630105961241 29 -28.099842908477754 30 -26.196663362815798
		 31 -24.094023952106255 32 -21.816857159480154 33 -19.390095468068395 34 -16.838671361001889
		 35 -14.187517321411635 36 -11.461565832428555 37 -8.6857493771835639 38 -5.8850004388076655
		 39 -3.0842515004317574 40 -0.30843504518675852 41 2.4175164437963144 42 5.0686704833865814
		 43 7.6200945904530446 44 10.046856281864835 45 12.324023074490963 46 14.426662485200453
		 47 16.32984203086243 48 18.008629228345924 49 19.438091594519967 50 20.59329664625367
		 51 21.449311900416046 52 21.981204873876148 53 22.164043083503074 54 20.522262114354692
		 55 16.417809691483743 56 11.082021541751537 57 5.7462333920193069 58 1.6417809691483645
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 -0.3810952202664637 79 -1.4581034514542961 80 -3.1316085491461809 81 -5.3021943689247415
		 82 -7.870444766372656 83 -10.736943597072621 84 -13.802274716607227 85 -16.967021980559174
		 86 -20.131769244511158 87 -23.197100364045763 88 -26.063599194745706 89 -28.631849592193639
		 90 -30.802435411972194 91 -32.475940509664078 92 -33.552948740851932 93 -33.934043961118384
		 94 -33.751205751491483 95 -33.219312778031359 96 -32.363297523868987 97 -31.208092472135302
		 98 -29.778630105961241 99 -28.099842908477754 100 -26.196663362815798 101 -24.094023952106255
		 102 -21.816857159480154 103 -19.390095468068395 104 -16.838671361001889 105 -14.187517321411635
		 106 -11.461565832428555 107 -8.6857493771835639 108 -5.8850004388076655 109 -3.0842515004317574
		 110 -0.30843504518675852 111 2.4175164437963144 112 5.0686704833865814 113 7.6200945904530446
		 114 10.046856281864835 115 12.324023074490963 116 14.426662485200453 117 16.32984203086243
		 118 18.008629228345924 119 19.438091594519967 120 20.59329664625367 121 21.449311900416046
		 122 21.981204873876148 123 22.164043083503074 124 20.522262114354692 125 16.417809691483743
		 126 11.082021541751537 127 5.7462333920193069 128 1.6417809691483645 129 0 130 0
		 131 0 132 0 133 0 134 0 135 0 136 0 137 0 138 0 139 0 140 0 141 0 142 0 143 0 144 0
		 145 0 146 0 147 0 148 -0.3810952202664637 149 -1.4581034514542961 150 -3.1316085491461809
		 151 -5.3021943689247415 152 -7.870444766372656 153 -10.736943597072621 154 -13.802274716607227
		 155 -16.967021980559174 156 -20.131769244511158;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_bk_foot_001_cnt_rotateY";
	rename -uid "0282D509-4FCC-620C-954C-3390CB46AB57";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0 79 0 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0 88 0 89 0 90 0 91 0 92 0
		 93 0 94 0 95 0 96 0 97 0 98 0 99 0 100 0 101 0 102 0 103 0 104 0 105 0 106 0 107 0
		 108 0 109 0 110 0 111 0 112 0 113 0 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0
		 122 0 123 0 124 0 125 0 126 0 127 0 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0
		 136 0 137 0 138 0 139 0 140 0 141 0 142 0 143 0 144 0 145 0 146 0 147 0 148 0 149 0
		 150 0 151 0 152 0 153 0 154 0 155 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_bk_foot_001_cnt_rotateZ";
	rename -uid "2C3D5F8A-4E12-BA3E-877E-D28FF707678E";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0
		 25 0 26 0 27 0 28 0 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0
		 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0
		 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0
		 76 0 77 0 78 0 79 0 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0 88 0 89 0 90 0 91 0 92 0
		 93 0 94 0 95 0 96 0 97 0 98 0 99 0 100 0 101 0 102 0 103 0 104 0 105 0 106 0 107 0
		 108 0 109 0 110 0 111 0 112 0 113 0 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0
		 122 0 123 0 124 0 125 0 126 0 127 0 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0
		 136 0 137 0 138 0 139 0 140 0 141 0 142 0 143 0 144 0 145 0 146 0 147 0 148 0 149 0
		 150 0 151 0 152 0 153 0 154 0 155 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_ft_foot_001_cnt_translateX";
	rename -uid "D4646965-4262-B534-976E-74AEBE71B41A";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 -0.10806947702513625 3 -0.18737972422464821
		 4 -0.28536864646554605 5 -0.40025732231531386 6 -0.53026683034143751 7 -0.67361824911140289
		 8 -0.82853265719270119 9 -0.9932311331528183 10 -1.1659347555592436 11 -1.3448646029794613
		 12 -1.5282417539809607 13 -1.7142872871312278 14 -1.9012222809977501 15 -2.0872678141480172
		 16 -2.2706449651495184 17 -2.4495748125697361 18 -2.6222784349761596 19 -2.7869769109362785
		 20 -2.9418913190175768 21 -3.085242737787544 22 -3.2152522458136676 23 -3.3301409216634337
		 24 -3.4281298439043333 25 -3.5074400911038488 26 -3.5662927418294714 27 -3.602908874648687
		 28 -3.6155095681289833 29 -3.5142753002213727 30 -3.2394965730435672 31 -2.8345595014131266
		 32 -2.3428502001475824 33 -1.8077547840644907 34 -1.272659367981408 35 -0.78095006671586376
		 36 -0.3760129950854143 37 -0.10123426790761769 38 -7.1054273576010019e-15 39 -7.1054273576010019e-15
		 40 -7.1054273576010019e-15 41 -7.1054273576010019e-15 42 -3.5527136788005009e-15
		 43 -3.5527136788005009e-15 44 -3.5527136788005009e-15 45 -3.5527136788005009e-15
		 46 -3.5527136788005009e-15 47 -3.5527136788005009e-15 48 -3.5527136788005009e-15
		 49 -3.5527136788005009e-15 50 -3.5527136788005009e-15 51 -3.5527136788005009e-15
		 52 -3.5527136788005009e-15 53 -3.5527136788005009e-15 54 -3.5527136788005009e-15
		 55 -3.5527136788005009e-15 56 0 57 0 58 0 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0
		 67 0 68 0 69 0 70 -0.01260069348029802 71 -0.049216826299510075 72 -0.10806947702513625
		 73 -0.18737972422464821 74 -0.28536864646554605 75 -0.40025732231531386 76 -0.53026683034143751
		 77 -0.67361824911140289 78 -0.82853265719270119 79 -0.9932311331528183 80 -1.1659347555592436
		 81 -1.3448646029794613 82 -1.5282417539809607 83 -1.7142872871312278 84 -1.9012222809977501
		 85 -2.0872678141480172 86 -2.2706449651495184 87 -2.4495748125697361 88 -2.6222784349761596
		 89 -2.7869769109362785 90 -2.9418913190175768 91 -3.085242737787544 92 -3.2152522458136676
		 93 -3.3301409216634337 94 -3.4281298439043333 95 -3.5074400911038488 96 -3.5662927418294714
		 97 -3.602908874648687 98 -3.6155095681289833 99 -3.5142753002213727 100 -3.2394965730435672
		 101 -2.8345595014131266 102 -2.3428502001475824 103 -1.8077547840644907 104 -1.272659367981408
		 105 -0.78095006671586376 106 -0.3760129950854143 107 -0.10123426790761769 108 -7.1054273576010019e-15
		 109 -7.1054273576010019e-15 110 -7.1054273576010019e-15 111 -7.1054273576010019e-15
		 112 -3.5527136788005009e-15 113 -3.5527136788005009e-15 114 -3.5527136788005009e-15
		 115 -3.5527136788005009e-15 116 -3.5527136788005009e-15 117 -3.5527136788005009e-15
		 118 -3.5527136788005009e-15 119 -3.5527136788005009e-15 120 -3.5527136788005009e-15
		 121 -3.5527136788005009e-15 122 -3.5527136788005009e-15 123 -3.5527136788005009e-15
		 124 -3.5527136788005009e-15 125 -3.5527136788005009e-15 126 0 127 0 128 0 129 0 130 0
		 131 0 132 0 133 0 134 0 135 0 136 0 137 0 138 0 139 0 140 -0.01260069348029802 141 -0.049216826299510075;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_ft_foot_001_cnt_translateY";
	rename -uid "48CE73A4-461B-931C-5562-EB97F1618DDE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 1.1245419727081369 3 1.8087836275735398
		 4 2.5320574168668912 5 3.2386775889548183 6 3.8864971805763489 7 4.4465897358475814
		 8 4.9008475374867615 9 5.2381477283865649 10 5.4497151037234879 11 5.5240111608768174
		 12 5.5005410129743311 13 5.4343309818716357 14 5.3316775088473634 15 5.198862533313644
		 16 5.0421366647560362 17 4.8676953825745919 18 4.6816479093646421 19 4.4899781881387355
		 20 4.2984971339889491 21 4.1127850344329815 22 3.9381226530959061 23 3.7794092668234995
		 24 3.6410655621861885 25 3.5269190679346529 26 3.4400696508879705 27 3.3827326135151594
		 28 3.3560571785958047 29 3.4278226718997438 30 3.5815980788310462 31 3.6848927124961826
		 32 3.6226548825291758 33 3.3270010855321956 34 2.79852617425178 35 2.112880639713584
		 36 1.4095875104463538 37 0.86470054509526051 38 0.65138471208848614 39 0.92431942437824333
		 40 1.0354601993502754 41 0.47317758246330754 42 -1.7763568394002505e-15 43 -1.7763568394002505e-15
		 44 -1.7763568394002505e-15 45 -1.7763568394002505e-15 46 -1.7763568394002505e-15
		 47 -1.7763568394002505e-15 48 -1.7763568394002505e-15 49 -1.7763568394002505e-15
		 50 -1.7763568394002505e-15 51 -1.7763568394002505e-15 52 -1.7763568394002505e-15
		 53 -1.7763568394002505e-15 54 -1.7763568394002505e-15 55 -1.7763568394002505e-15
		 56 -1.7763568394002505e-15 57 -1.7763568394002505e-15 58 -1.7763568394002505e-15
		 59 -1.7763568394002505e-15 60 -1.7763568394002505e-15 61 -1.7763568394002505e-15
		 62 -1.7763568394002505e-15 63 -1.7763568394002505e-15 64 -1.7763568394002505e-15
		 65 -1.7763568394002505e-15 66 -1.7763568394002505e-15 67 -1.7763568394002505e-15
		 68 -1.7763568394002505e-15 69 -1.7763568394002505e-15 70 0.14746286845270262 71 0.54632491238537639
		 72 1.1245419727081369 73 1.8087836275735398 74 2.5320574168668912 75 3.2386775889548183
		 76 3.8864971805763489 77 4.4465897358475814 78 4.9008475374867615 79 5.2381477283865649
		 80 5.4497151037234879 81 5.5240111608768174 82 5.5005410129743311 83 5.4343309818716357
		 84 5.3316775088473634 85 5.198862533313644 86 5.0421366647560362 87 4.8676953825745919
		 88 4.6816479093646421 89 4.4899781881387355 90 4.2984971339889491 91 4.1127850344329815
		 92 3.9381226530959061 93 3.7794092668234995 94 3.6410655621861885 95 3.5269190679346529
		 96 3.4400696508879705 97 3.3827326135151594 98 3.3560571785958047 99 3.4278226718997438
		 100 3.5815980788310462 101 3.6848927124961826 102 3.6226548825291758 103 3.3270010855321956
		 104 2.79852617425178 105 2.112880639713584 106 1.4095875104463538 107 0.86470054509526051
		 108 0.65138471208848614 109 0.92431942437824333 110 1.0354601993502754 111 0.47317758246330754
		 112 -1.7763568394002505e-15 113 -1.7763568394002505e-15 114 -1.7763568394002505e-15
		 115 -1.7763568394002505e-15 116 -1.7763568394002505e-15 117 -1.7763568394002505e-15
		 118 -1.7763568394002505e-15 119 -1.7763568394002505e-15 120 -1.7763568394002505e-15
		 121 -1.7763568394002505e-15 122 -1.7763568394002505e-15 123 -1.7763568394002505e-15
		 124 -1.7763568394002505e-15 125 -1.7763568394002505e-15 126 -1.7763568394002505e-15
		 127 -1.7763568394002505e-15 128 -1.7763568394002505e-15 129 -1.7763568394002505e-15
		 130 -1.7763568394002505e-15 131 -1.7763568394002505e-15 132 -1.7763568394002505e-15
		 133 -1.7763568394002505e-15 134 -1.7763568394002505e-15 135 -1.7763568394002505e-15
		 136 -1.7763568394002505e-15 137 -1.7763568394002505e-15 138 -1.7763568394002505e-15
		 139 -1.7763568394002505e-15 140 0.14746286845270262 141 0.54632491238537639;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_ft_foot_001_cnt_translateZ";
	rename -uid "9616066D-4036-F816-7E03-23B8001E6D1B";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 24.013379050893974 3 24.766152416757599
		 4 25.325275816893914 5 25.736386721364333 6 26.038384932986148 7 26.260778118939417
		 8 26.422507497353408 9 26.532975174074245 10 26.595689621561341 11 26.614558212115199
		 12 26.381306358901476 13 25.707362156869969 14 24.631447686466604 15 23.192302498010662
		 16 21.428703690133304 17 19.379493817936581 18 17.083616329162314 19 14.580158031616937
		 20 11.908397841254319 21 9.1078607383682737 22 6.2183754564707101 23 3.2801339281413782
		 24 0.33374989449802328 25 -2.5796866726849359 26 -5.4185636086310858 27 -8.140714433244943
		 28 -10.70340676072024 29 -13.464168291632275 30 -16.649647801780858 31 -20.030884795725385
		 32 -23.411765203362137 33 -26.636179088611314 34 -29.577478844887224 35 -32.117646641957094
		 36 -34.127025264156721 37 -35.454679860083786 38 -35.934372958662664 39 -35.881570273050592
		 40 -35.594134756805758 41 -34.715840749820657 42 -32.975563405995146 43 -30.795555718931695
		 44 -28.623473618307045 45 -26.460660825749244 46 -24.308461062886323 47 -22.168218051346372
		 48 -20.041275512757409 49 -17.928977168747494 50 -15.832666740944662 51 -13.753687950976984
		 52 -11.693384520472522 53 -9.6531001710592736 54 -7.6341786243653491 55 -5.6379636020187736
		 56 -3.6657988256475704 57 -1.7190280168798466 58 0.2010051026563886 59 2.0929568113330959
		 60 3.9554833875221851 61 5.7872411095956187 62 7.5868862559253776 63 9.3530751048833576
		 64 11.084463934841557 65 12.829419237096225 66 14.623846149825972 67 16.447708921726289
		 68 18.280971801492676 69 20.103599037820604 70 21.730233264454046 71 23.017862633793634
		 72 24.013379050893974 73 24.766152416757599 74 25.325275816893914 75 25.736386721364333
		 76 26.038384932986148 77 26.260778118939417 78 26.422507497353408 79 26.532975174074245
		 80 26.595689621561341 81 26.614558212115199 82 26.381306358901476 83 25.707362156869969
		 84 24.631447686466604 85 23.192302498010662 86 21.428703690133304 87 19.379493817936581
		 88 17.083616329162314 89 14.580158031616937 90 11.908397841254319 91 9.1078607383682737
		 92 6.2183754564707101 93 3.2801339281413782 94 0.33374989449802328 95 -2.5796866726849359
		 96 -5.4185636086310858 97 -8.140714433244943 98 -10.70340676072024 99 -13.464168291632275
		 100 -16.649647801780858 101 -20.030884795725385 102 -23.411765203362137 103 -26.636179088611314
		 104 -29.577478844887224 105 -32.117646641957094 106 -34.127025264156721 107 -35.454679860083786
		 108 -35.934372958662664 109 -35.881570273050592 110 -35.594134756805758 111 -34.715840749820657
		 112 -32.975563405995146 113 -30.795555718931695 114 -28.623473618307045 115 -26.460660825749244
		 116 -24.308461062886323 117 -22.168218051346372 118 -20.041275512757409 119 -17.928977168747494
		 120 -15.832666740944662 121 -13.753687950976984 122 -11.693384520472522 123 -9.6531001710592736
		 124 -7.6341786243653491 125 -5.6379636020187736 126 -3.6657988256475704 127 -1.7190280168798466
		 128 0.2010051026563886 129 2.0929568113330959 130 3.9554833875221851 131 5.7872411095956187
		 132 7.5868862559253776 133 9.3530751048833576 134 11.084463934841557 135 12.829419237096225
		 136 14.623846149825972 137 16.447708921726289 138 18.280971801492676 139 20.103599037820604
		 140 21.730233264454046 141 23.017862633793634;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_ft_foot_001_cnt_rotateX";
	rename -uid "CDB87A78-41AB-8A27-B4F1-DD894EF899F4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 -4.6896721955904672 3 -7.78138201342417
		 4 -11.289951581977062 5 -15.006951025889492 6 -18.723950469801927 7 -22.232520038354817
		 8 -25.324229856188524 9 -27.790650047943497 10 -29.423350738260186 11 -30.013902051778985
		 12 -30.011541176145546 13 -29.99501504671149 14 -29.950158409676202 15 -29.862806011239044
		 16 -29.718792597599428 17 -29.503952914956706 18 -29.204121709510257 19 -28.80513372745947
		 20 -28.292823715003742 21 -27.653026418342446 22 -26.871576583674941 23 -25.934308957200624
		 24 -24.827058285118888 25 -23.535659313629079 26 -22.045946788930614 27 -20.343755457222851
		 28 -18.41492006470521 29 -15.313815560340233 30 -10.434708763877131 31 -4.2732722276953314
		 32 2.6748214958261731 33 9.9138998543079921 34 16.9482902953707 35 23.282320266635349
		 36 28.420317215722523 37 31.866608590252891 38 33.125521837847316 39 27.949659050683657
		 40 16.562760918923619 41 5.1758627871636564 42 0 43 0 44 0 45 0 46 0 47 0 48 0 49 0
		 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0 59 0 60 0 61 0 62 0 63 0 64 0 65 0 66 0
		 67 0 68 0 69 0 70 -0.59055131351879564 71 -2.2232520038354879 72 -4.6896721955904672
		 73 -7.78138201342417 74 -11.289951581977062 75 -15.006951025889492 76 -18.723950469801927
		 77 -22.232520038354817 78 -25.324229856188524 79 -27.790650047943497 80 -29.423350738260186
		 81 -30.013902051778985 82 -30.011541176145546 83 -29.99501504671149 84 -29.950158409676202
		 85 -29.862806011239044 86 -29.718792597599428 87 -29.503952914956706 88 -29.204121709510257
		 89 -28.80513372745947 90 -28.292823715003742 91 -27.653026418342446 92 -26.871576583674941
		 93 -25.934308957200624 94 -24.827058285118888 95 -23.535659313629079 96 -22.045946788930614
		 97 -20.343755457222851 98 -18.41492006470521 99 -15.313815560340233 100 -10.434708763877131
		 101 -4.2732722276953314 102 2.6748214958261731 103 9.9138998543079921 104 16.9482902953707
		 105 23.282320266635349 106 28.420317215722523 107 31.866608590252891 108 33.125521837847316
		 109 27.949659050683657 110 16.562760918923619 111 5.1758627871636564 112 0 113 0
		 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0 122 0 123 0 124 0 125 0 126 0 127 0
		 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0 136 0 137 0 138 0 139 0 140 -0.59055131351879564
		 141 -2.2232520038354879;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_ft_foot_001_cnt_rotateY";
	rename -uid "B11E3972-4B21-A749-3BF4-C583B94794A5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 0 3 0 4 0 5 0 6 0 7 0 8 0 9 0 10 0 11 0
		 12 0 13 0 14 0 15 0 16 0 17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0 25 0 26 0 27 0 28 0
		 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0 42 0 43 0 44 0 45 0
		 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0 59 0 60 0 61 0 62 0
		 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0 76 0 77 0 78 0 79 0
		 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0 88 0 89 0 90 0 91 0 92 0 93 0 94 0 95 0 96 0
		 97 0 98 0 99 0 100 0 101 0 102 0 103 0 104 0 105 0 106 0 107 0 108 0 109 0 110 0
		 111 0 112 0 113 0 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0 122 0 123 0 124 0
		 125 0 126 0 127 0 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0 136 0 137 0 138 0
		 139 0 140 0 141 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_ft_foot_001_cnt_rotateZ";
	rename -uid "5BC2760A-4A88-19B3-5A50-7EBE61FD1495";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 0 3 0 4 0 5 0 6 0 7 0 8 0 9 0 10 0 11 0
		 12 0 13 0 14 0 15 0 16 0 17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0 25 0 26 0 27 0 28 0
		 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0 42 0 43 0 44 0 45 0
		 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0 59 0 60 0 61 0 62 0
		 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0 73 0 74 0 75 0 76 0 77 0 78 0 79 0
		 80 0 81 0 82 0 83 0 84 0 85 0 86 0 87 0 88 0 89 0 90 0 91 0 92 0 93 0 94 0 95 0 96 0
		 97 0 98 0 99 0 100 0 101 0 102 0 103 0 104 0 105 0 106 0 107 0 108 0 109 0 110 0
		 111 0 112 0 113 0 114 0 115 0 116 0 117 0 118 0 119 0 120 0 121 0 122 0 123 0 124 0
		 125 0 126 0 127 0 128 0 129 0 130 0 131 0 132 0 133 0 134 0 135 0 136 0 137 0 138 0
		 139 0 140 0 141 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_ft_foot_001_cnt_translateX";
	rename -uid "CF5CB28B-404A-6192-A0B2-8C9FFC18181A";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 0 3 0 4 0 5 0 6 0 7 0 8 0 9 0 10 0 11 0
		 12 0 13 0 14 0 15 0 16 0 17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0 25 0 26 0 27 0 28 0
		 29 0 30 0 31 0 32 0 33 0 34 0.01260069348029802 35 0.049216826299513627 36 0.10806947702513625
		 37 0.18737972422465177 38 0.2853686464655496 39 0.40025732231531919 40 0.53026683034144284
		 41 0.67361824911140644 42 0.82853265719270297 43 0.99323113315282185 44 1.165934755559249
		 45 1.3448646029794631 46 1.5282417539809643 47 1.7142872871312278 48 1.9012222809977537
		 49 2.0872678141480208 50 2.270644965149522 51 2.4495748125697361 52 2.6222784349761596
		 53 2.7869769109362785 54 2.9418913190175751 55 3.0852427377875422 56 3.2152522458136694
		 57 3.3301409216634354 58 3.4281298439043297 59 3.5074400911038488 60 3.5662927418294714
		 61 3.602908874648687 62 3.6155095681289815 63 3.5142753002213745 64 3.2394965730435707
		 65 2.8345595014131248 66 2.3428502001475842 67 1.8077547840644961 68 1.2726593679814009
		 69 0.7809500667158602 70 0.3760129950854143 71 0.10123426790761059 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_ft_foot_001_cnt_translateY";
	rename -uid "917BA5A5-4A6A-EEE0-AE28-7B9996BE9590";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 0.65137062673790069 3 0.92430779651055062
		 4 1.035453713153025 5 0.47317570995353364 6 -1.7763568394002505e-15 7 -1.7763568394002505e-15
		 8 -1.7763568394002505e-15 9 -1.7763568394002505e-15 10 -1.7763568394002505e-15 11 -1.7763568394002505e-15
		 12 -1.7763568394002505e-15 13 -1.7763568394002505e-15 14 -1.7763568394002505e-15
		 15 -1.7763568394002505e-15 16 -1.7763568394002505e-15 17 -1.7763568394002505e-15
		 18 -1.7763568394002505e-15 19 -1.7763568394002505e-15 20 -1.7763568394002505e-15
		 21 -1.7763568394002505e-15 22 -1.7763568394002505e-15 23 -1.7763568394002505e-15
		 24 -1.7763568394002505e-15 25 -1.7763568394002505e-15 26 -1.7763568394002505e-15
		 27 -1.7763568394002505e-15 28 -1.7763568394002505e-15 29 -1.7763568394002505e-15
		 30 -1.7763568394002505e-15 31 -1.7763568394002505e-15 32 -1.7763568394002505e-15
		 33 -1.7763568394002505e-15 34 0.14746307206133835 35 0.54632566773324953 36 1.1245435295926285
		 37 1.8087861328861763 38 2.5320609193442252 39 3.2386820520668032 40 3.8865025018326484
		 41 4.4465957698411422 42 4.9008541190430108 43 5.238154691557547 44 5.4497222921761761
		 45 5.5240184254072418 46 5.5005482772063559 47 5.4343382440135706 48 5.3316847653048001
		 49 5.1988697786535205 50 5.0421438916292018 51 4.8677025815806054 52 4.6816550688423497
		 53 4.4899852938669298 54 4.2985041687816974 55 4.1127919776151387 56 3.9381294798454931
		 57 3.7794159473694151 58 3.6410720608521459 59 3.5269253420235813 60 3.4400756494058555
		 61 3.3827382757396549 62 3.3560624325033181 63 3.4278272097911859 64 3.5816013461890464
		 65 3.6848941368084382 66 3.6226539341656014 67 3.3269973696368602 68 2.7985195215150753
		 69 2.1128711706069083 70 1.4095756616014832 71 0.86468706260722072 72 0.65137062673790069;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_ft_foot_001_cnt_translateZ";
	rename -uid "E3424B94-4B91-9132-32E6-6FB4BD8CFA87";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 35.934365312231115 3 35.881563263133785
		 4 35.594129908512045 5 34.715839035737716 6 32.975563405995146 7 30.795555718931709
		 8 28.623473618307045 9 26.460660825749244 10 24.308461062886323 11 22.168218051346372
		 12 20.041275512757409 13 17.928977168747494 14 15.832666740944658 15 13.75368795097698
		 16 11.693384520472504 17 9.6531001710592772 18 7.6341786243653456 19 5.6379636020187629
		 20 3.6657988256475775 21 1.7190280168798395 22 -0.20100510265639571 23 -2.0929568113330959
		 24 -3.955483387522186 25 -5.7872411095956267 26 -7.5868862559253571 27 -9.3530751048833594
		 28 -11.084463934841558 29 -12.829419237096204 30 -14.623846149825958 31 -16.447708921726289
		 32 -18.280971801492662 33 -20.10359903782059 34 -21.730233058313871 35 -23.01786184694005
		 36 -24.013377357584559 37 -24.766149539860393 38 -25.325271537141322 39 -25.73638089183428
		 40 -26.038377494662306 41 -26.260769113980231 42 -26.422497077978761 43 -26.532963606442955
		 44 -26.5956772852348 45 -26.614545596213219 46 -26.381293744118722 47 -25.707349549921446
		 48 -24.631435100779342 49 -23.19228995371413 50 -21.428691214038388 51 -19.379481443498243
		 52 -17.083604096420167 53 -14.580145987106688 54 -11.908386037881471 55 -9.1078492352329459
		 56 -6.2183643186292752 57 -3.2801232262866566 58 -0.33373970453496149 59 2.5796962701970578
		 60 5.4185725292054236 61 8.1407225894055788 62 10.703414063203342 63 13.464174251835521
		 64 16.64965173398874 65 20.030886333592058 66 23.411764296411491 67 26.636175959341877
		 68 29.577473906909319 69 32.11764039406966 70 34.127018187212592 71 35.454672347995349
		 72 35.934365312231115;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_ft_foot_001_cnt_rotateX";
	rename -uid "72AB192D-4520-7DED-3407-00AAD5A13D6F";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 -33.125521837847316 3 -27.949659050683678
		 4 -16.562760918923654 5 -5.175862787163644 6 0 7 0 8 0 9 0 10 0 11 0 12 0 13 0 14 0
		 15 0 16 0 17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0 25 0 26 0 27 0 28 0 29 0 30 0 31 0
		 32 0 33 0 34 0.59055131351880152 35 2.2232520038354777 36 4.6896721955904672 37 7.7813820134241896
		 38 11.289951581977041 39 15.006951025889492 40 18.723950469801949 41 22.232520038354803
		 42 25.324229856188524 43 27.790650047943512 44 29.423350738260186 45 30.013902051778985
		 46 30.011541176145546 47 29.99501504671149 48 29.950158409676209 49 29.862806011239044
		 50 29.718792597599428 51 29.503952914956706 52 29.204121709510257 53 28.80513372745947
		 54 28.292823715003745 55 27.653026418342449 56 26.871576583674941 57 25.934308957200631
		 58 24.827058285118898 59 23.53565931362909 60 22.045946788930632 61 20.343755457222887
		 62 18.41492006470521 63 15.313815560340272 64 10.434708763877238 65 4.2732722276953075
		 66 -2.6748214958261287 67 -9.9138998543078731 68 -16.948290295370747 69 -23.282320266635345
		 70 -28.420317215722477 71 -31.866608590252909 72 -33.125521837847316;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_ft_foot_001_cnt_rotateY";
	rename -uid "6A99F9CF-46A8-8A0C-C44B-EAAD6C3E2FD0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 0 3 0 4 0 5 0 6 0 7 0 8 0 9 0 10 0 11 0
		 12 0 13 0 14 0 15 0 16 0 17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0 25 0 26 0 27 0 28 0
		 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0 42 0 43 0 44 0 45 0
		 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0 59 0 60 0 61 0 62 0
		 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_ft_foot_001_cnt_rotateZ";
	rename -uid "0C69A4EB-40DB-2405-2F42-FBA96621E843";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 71 ".ktv[0:70]"  2 0 3 0 4 0 5 0 6 0 7 0 8 0 9 0 10 0 11 0
		 12 0 13 0 14 0 15 0 16 0 17 0 18 0 19 0 20 0 21 0 22 0 23 0 24 0 25 0 26 0 27 0 28 0
		 29 0 30 0 31 0 32 0 33 0 34 0 35 0 36 0 37 0 38 0 39 0 40 0 41 0 42 0 43 0 44 0 45 0
		 46 0 47 0 48 0 49 0 50 0 51 0 52 0 53 0 54 0 55 0 56 0 57 0 58 0 59 0 60 0 61 0 62 0
		 63 0 64 0 65 0 66 0 67 0 68 0 69 0 70 0 71 0 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_neck_jnt_001_cnt_translateX";
	rename -uid "2D5C0AC8-4A67-49C5-EFAC-6CB77C784F33";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 -1.4820311693640491 3 -1.525565195259027
		 4 -1.563520500539969 5 -1.5284995924556668 6 -1.4514360859335014 7 -1.3425406243786782
		 8 -1.2120348938655319 9 -1.0704411708691666 10 -0.92875302621095557 11 -0.79849021513321361
		 12 -0.69164219079533851 13 -0.62050481400349256 14 -0.59741347743820583 15 -0.54224167790973965
		 16 -0.38201662046495244 17 -0.13846406542904433 18 0.16643148231871052 19 0.5087375577401545
		 20 0.86160905578555358 21 1.1950769834003587 22 1.4764351428588505 23 1.6712181579106407
		 24 1.7447140286082856 25 1.6823503040078691 26 1.5083966836393152 27 1.2484760750045112
		 28 0.92859765866063171 29 0.57405837841575647 30 0.20860649078937143 31 -0.14611980262332835
		 32 -0.47084788163887836 33 -0.74858344189281922 34 -0.9641470188420378 35 -1.103311802788209
		 36 -1.1515365093949796 37 -1.262903146171201 38 -1.3628811517525001 39 -1.3176974761868507
		 40 -1.2351901708356472 41 -1.1288790155191109 42 -1.0123983091526156 43 -0.89971401629227898
		 44 -0.80519572112092774 45 -0.74354661342907136 46 -0.72959451447069412 47 -0.67090603168544405
		 48 -0.4857250897493266 49 -0.20336686952764893 50 0.14654814578317499 51 0.53206709375328387
		 52 0.91776529586297073 53 1.2723632530487734 54 1.5661041586871107 55 1.7615221456548582
		 56 1.8207576510629764 57 1.7607892643452985 58 1.6304579221303506 59 1.4413517290054685
		 60 1.2054301346467504 61 0.93468778250428031 62 0.64085703401068983 63 0.33515638302541717
		 64 0.028095361858245127 65 -0.27065264428769353 66 -0.55229961307793829 67 -0.80903065749055258
		 68 -1.0337430669552141 69 -1.2197723556156959 70 -1.360783422674718 71 -1.4504455066058455
		 72 -1.4820311693640491 73 -1.525565195259027 74 -1.563520500539969 75 -1.5284995924556668
		 76 -1.4514360859335014 77 -1.3425406243786782 78 -1.2120348938655319 79 -1.0704411708691666
		 80 -0.92875302621095557 81 -0.79849021513321361 82 -0.69164219079533851 83 -0.62050481400349256
		 84 -0.59741347743820583 85 -0.54224167790973965 86 -0.38201662046495244 87 -0.13846406542904433
		 88 0.16643148231871052 89 0.5087375577401545 90 0.86160905578555358 91 1.1950769834003587
		 92 1.4764351428588505 93 1.6712181579106407 94 1.7447140286082856 95 1.6823503040078691
		 96 1.5083966836393152 97 1.2484760750045112 98 0.92859765866063171 99 0.57405837841575647
		 100 0.20860649078937143 101 -0.14611980262332835 102 -0.47084788163887836 103 -0.74858344189281922
		 104 -0.9641470188420378 105 -1.103311802788209 106 -1.1515365093949796 107 -1.262903146171201
		 108 -1.3628811517525001 109 -1.3176974761868507 110 -1.2351901708356472 111 -1.1288790155191109
		 112 -1.0123983091526156 113 -0.89971401629227898 114 -0.80519572112092774 115 -0.74354661342907136
		 116 -0.72959451447069412 117 -0.67090603168544405 118 -0.4857250897493266 119 -0.20336686952764893
		 120 0.14654814578317499 121 0.53206709375328387 122 0.91776529586297073 123 1.2723632530487734
		 124 1.5661041586871107 125 1.7615221456548582 126 1.8207576510629764 127 1.7607892643452985
		 128 1.6304579221303506 129 1.4413517290054685 130 1.2054301346467504 131 0.93468778250428031
		 132 0.64085703401068983 133 0.33515638302541717 134 0.028095361858245127 135 -0.27065264428769353
		 136 -0.55229961307793829 137 -0.80903065749055258 138 -1.0337430669552141 139 -1.2197723556156959
		 140 -1.360783422674718 141 -1.4504455066058455;
createNode animCurveTL -n "cn_neck_jnt_001_cnt_translateY";
	rename -uid "BF88103B-46AF-CA16-5A77-DABCF3884271";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 -2.3527523557759338 3 -2.495524264197357
		 4 -2.7509176675877143 5 -2.978185689031605 6 -3.2064114771820869 7 -3.4301369185696728
		 8 -3.6438962071415801 9 -3.8420601895831084 10 -4.01873601961465 11 -4.1677255595641753
		 12 -4.2825485491037512 13 -4.3565368660775619 14 -4.3830043053594778 15 -4.2720883678562132
		 16 -3.9681769066310153 17 -3.5169764701906843 18 -2.9642212765558753 19 -2.3545065217280268
		 20 -1.7304655130124615 21 -1.1323461420906682 22 -0.59808019202792195 23 -0.16392421444972172
		 24 0.13427806357945826 25 0.28014265973470742 26 0.29556212431367612 27 0.20040306504182581
		 28 0.014267526857409507 29 -0.24253209003175868 30 -0.54833597921994226 31 -0.87982624113229235
		 32 -1.212008158761293 33 -1.518435997371796 34 -1.7716341143703573 35 -1.9436751632081695
		 36 -2.006902027365939 37 -2.3180612139124577 38 -2.8262261872796159 39 -3.2491059516220702
		 40 -3.7140424996257622 41 -4.1910292914771645 42 -4.6499884942087135 43 -5.0606574140417138
		 44 -5.3925529925993771 45 -5.6150190375099527 46 -5.697361157295461 47 -5.7302488488181069
		 48 -5.7885727730040202 49 -5.8191605148363408 50 -5.7689624333956289 51 -5.5836987540522642
		 52 -5.2068913939254173 53 -4.7032774186718598 54 -4.2074162637302663 55 -3.7643098254491179
		 56 -3.4186080559724843 57 -3.1558855734950413 58 -2.9299187047114259 59 -2.7390625049546742
		 60 -2.5819089537418165 61 -2.456992591054771 62 -2.3625618118718279 63 -2.2964169740438791
		 64 -2.2558128182973292 65 -2.2374199277604561 66 -2.2373380837603634 67 -2.251161424055752
		 68 -2.2740627704442602 69 -2.300911790087639 70 -2.3264341552350913 71 -2.3453854557778726
		 72 -2.3527523557759338 73 -2.495524264197357 74 -2.7509176675877143 75 -2.978185689031605
		 76 -3.2064114771820869 77 -3.4301369185696728 78 -3.6438962071415801 79 -3.8420601895831084
		 80 -4.01873601961465 81 -4.1677255595641753 82 -4.2825485491037512 83 -4.3565368660775619
		 84 -4.3830043053594778 85 -4.2720883678562132 86 -3.9681769066310153 87 -3.5169764701906843
		 88 -2.9642212765558753 89 -2.3545065217280268 90 -1.7304655130124615 91 -1.1323461420906682
		 92 -0.59808019202792195 93 -0.16392421444972172 94 0.13427806357945826 95 0.28014265973470742
		 96 0.29556212431367612 97 0.20040306504182581 98 0.014267526857409507 99 -0.24253209003175868
		 100 -0.54833597921994226 101 -0.87982624113229235 102 -1.212008158761293 103 -1.518435997371796
		 104 -1.7716341143703573 105 -1.9436751632081695 106 -2.006902027365939 107 -2.3180612139124577
		 108 -2.8262261872796159 109 -3.2491059516220702 110 -3.7140424996257622 111 -4.1910292914771645
		 112 -4.6499884942087135 113 -5.0606574140417138 114 -5.3925529925993771 115 -5.6150190375099527
		 116 -5.697361157295461 117 -5.7302488488181069 118 -5.7885727730040202 119 -5.8191605148363408
		 120 -5.7689624333956289 121 -5.5836987540522642 122 -5.2068913939254173 123 -4.7032774186718598
		 124 -4.2074162637302663 125 -3.7643098254491179 126 -3.4186080559724843 127 -3.1558855734950413
		 128 -2.9299187047114259 129 -2.7390625049546742 130 -2.5819089537418165 131 -2.456992591054771
		 132 -2.3625618118718279 133 -2.2964169740438791 134 -2.2558128182973292 135 -2.2374199277604561
		 136 -2.2373380837603634 137 -2.251161424055752 138 -2.2740627704442602 139 -2.300911790087639
		 140 -2.3264341552350913 141 -2.3453854557778726;
createNode animCurveTL -n "cn_neck_jnt_001_cnt_translateZ";
	rename -uid "7EAFD5F5-4DEE-A0C1-A4CB-ACB1E4DF6FF2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 -0.022421396588204079 3 -0.013265142510737848
		 4 -0.0040848858022359824 5 0.0025078400538443013 6 0.0063869167659500459 7 0.0071185878701673921
		 8 0.0045284824611424579 9 -0.001248949081984696 10 -0.009738722681965406 11 -0.020117640290386873
		 12 -0.031237078064201285 13 -0.041681875899813647 14 -0.049869368230183841 15 -0.062868384998652882
		 16 -0.087982972997974862 17 -0.1233391487703579 18 -0.16620837836158175 19 -0.21307504753684503
		 20 -0.26004877395249881 21 -0.30313166934596181 22 -0.33850999163427042 23 -0.36291491022850808
		 24 -0.3739872615437525 25 -0.37207316054428574 26 -0.36116241794169168 27 -0.34515391322375244
		 28 -0.32757765875456846 29 -0.31136436473557971 30 -0.29869744617758176 31 -0.29094191284746751
		 32 -0.2886396879112787 33 -0.29155550980847755 34 -0.29875262226995375 35 -0.30867394384157909
		 36 -0.31920319804227759 37 -0.32990519947487212 38 -0.34121574001032257 39 -0.34917464844188206
		 40 -0.35427988403761113 41 -0.35629665357016505 42 -0.35537722815163142 43 -0.35204914165631807
		 44 -0.34718952570037453 45 -0.34197807592743579 46 -0.33782272748982556 47 -0.32894161547896417
		 48 -0.30890200973903292 49 -0.27882393483842183 50 -0.24065391009136272 51 -0.19718719860147349
		 52 -0.15198984720664832 53 -0.10991950755259738 54 -0.076660003091354234 55 -0.056218430487536164
		 56 -0.050412616272368815 57 -0.055129417395594515 58 -0.064188228001355041 59 -0.074561371015650746
		 60 -0.083696377215003226 61 -0.089761901497702579 62 -0.09178812947756837 63 -0.089706943725394089
		 64 -0.084301800418715031 65 -0.077080586121150291 66 -0.070086252755245493 67 -0.063838807920008844
		 68 -0.056950289682853997 69 -0.049118728540117312 70 -0.040401994787576945 71 -0.031231987769247016
		 72 -0.022421396588204079 73 -0.013265142510737848 74 -0.0040848858022359824 75 0.0025078400538443013
		 76 0.0063869167659500459 77 0.0071185878701673921 78 0.0045284824611424579 79 -0.001248949081984696
		 80 -0.009738722681965406 81 -0.020117640290386873 82 -0.031237078064201285 83 -0.041681875899813647
		 84 -0.049869368230183841 85 -0.062868384998652882 86 -0.087982972997974862 87 -0.1233391487703579
		 88 -0.16620837836158175 89 -0.21307504753684503 90 -0.26004877395249881 91 -0.30313166934596181
		 92 -0.33850999163427042 93 -0.36291491022850808 94 -0.3739872615437525 95 -0.37207316054428574
		 96 -0.36116241794169168 97 -0.34515391322375244 98 -0.32757765875456846 99 -0.31136436473557971
		 100 -0.29869744617758176 101 -0.29094191284746751 102 -0.2886396879112787 103 -0.29155550980847755
		 104 -0.29875262226995375 105 -0.30867394384157909 106 -0.31920319804227759 107 -0.32990519947487212
		 108 -0.34121574001032257 109 -0.34917464844188206 110 -0.35427988403761113 111 -0.35629665357016505
		 112 -0.35537722815163142 113 -0.35204914165631807 114 -0.34718952570037453 115 -0.34197807592743579
		 116 -0.33782272748982556 117 -0.32894161547896417 118 -0.30890200973903292 119 -0.27882393483842183
		 120 -0.24065391009136272 121 -0.19718719860147349 122 -0.15198984720664832 123 -0.10991950755259738
		 124 -0.076660003091354234 125 -0.056218430487536164 126 -0.050412616272368815 127 -0.055129417395594515
		 128 -0.064188228001355041 129 -0.074561371015650746 130 -0.083696377215003226 131 -0.089761901497702579
		 132 -0.09178812947756837 133 -0.089706943725394089 134 -0.084301800418715031 135 -0.077080586121150291
		 136 -0.070086252755245493 137 -0.063838807920008844 138 -0.056950289682853997 139 -0.049118728540117312
		 140 -0.040401994787576945 141 -0.031231987769247016;
createNode animCurveTA -n "cn_neck_jnt_001_cnt_rotateX";
	rename -uid "59BADD85-4C06-61B7-4B49-4FA5DB8DF91F";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 0.33771290495308187 3 0.35094224011499597
		 4 0.36312213954514966 5 0.36487993537863112 6 0.35604125096507394 7 0.3354185176074716
		 8 0.30286675108180772 9 0.25940479747193101 10 0.20725482210599197 11 0.14980089726035567
		 12 0.091467544177630186 13 0.037518740468055388 14 -0.0062230522233520571 15 -0.063243678040306159
		 16 -0.15730988535834534 17 -0.28038309880369355 18 -0.4215929537272588 19 -0.56865662928218541
		 20 -0.70946995683194325 21 -0.83229058262138356 22 -0.92679968092204856 23 -0.98478406447542954
		 24 -1.0006710416324673 25 -0.97617310699848503 26 -0.92287618656490078 27 -0.85191979687847408
		 28 -0.77361667463155304 29 -0.69699832713753596 30 -0.62939970112471322 31 -0.57610123735218333
		 32 -0.54004947344449272 33 -0.52167293900378886 34 -0.51880100036561683 35 -0.52668298559540538
		 36 -0.53809693089461064 37 -0.5514822492114716 38 -0.56832347340722444 39 -0.57396595872863998
		 40 -0.57015999699936293 41 -0.556078712588292 42 -0.53237852103318484 43 -0.50109220374139518
		 44 -0.46547597658604517 45 -0.42981143586011378 46 -0.39916315310378381 47 -0.3536238644278325
		 48 -0.27168595864544165 49 -0.15892163371631346 50 -0.023810030209658085 51 0.12288958431821759
		 52 0.26890819160991242 53 0.39638813907309939 54 0.48700089590143392 55 0.53146972863996012
		 56 0.52702231463350968 57 0.48742757906290135 58 0.43191304101556532 59 0.37060132065222962
		 60 0.31235903099995876 61 0.26411420352532144 62 0.23039071695465232 63 0.21306545544270852
		 64 0.21135424570726286 65 0.22203092272929803 66 0.23988092746267212 67 0.25768677773734749
		 68 0.27407841094672919 69 0.29066278057677725 70 0.30776603827690108 71 0.32430258548354929
		 72 0.33771290495308187 73 0.35094224011499597 74 0.36312213954514966 75 0.36487993537863112
		 76 0.35604125096507394 77 0.3354185176074716 78 0.30286675108180772 79 0.25940479747193101
		 80 0.20725482210599197 81 0.14980089726035567 82 0.091467544177630186 83 0.037518740468055388
		 84 -0.0062230522233520571 85 -0.063243678040306159 86 -0.15730988535834534 87 -0.28038309880369355
		 88 -0.4215929537272588 89 -0.56865662928218541 90 -0.70946995683194325 91 -0.83229058262138356
		 92 -0.92679968092204856 93 -0.98478406447542954 94 -1.0006710416324673 95 -0.97617310699848503
		 96 -0.92287618656490078 97 -0.85191979687847408 98 -0.77361667463155304 99 -0.69699832713753596
		 100 -0.62939970112471322 101 -0.57610123735218333 102 -0.54004947344449272 103 -0.52167293900378886
		 104 -0.51880100036561683 105 -0.52668298559540538 106 -0.53809693089461064 107 -0.5514822492114716
		 108 -0.56832347340722444 109 -0.57396595872863998 110 -0.57015999699936293 111 -0.556078712588292
		 112 -0.53237852103318484 113 -0.50109220374139518 114 -0.46547597658604517 115 -0.42981143586011378
		 116 -0.39916315310378381 117 -0.3536238644278325 118 -0.27168595864544165 119 -0.15892163371631346
		 120 -0.023810030209658085 121 0.12288958431821759 122 0.26890819160991242 123 0.39638813907309939
		 124 0.48700089590143392 125 0.53146972863996012 126 0.52702231463350968 127 0.48742757906290135
		 128 0.43191304101556532 129 0.37060132065222962 130 0.31235903099995876 131 0.26411420352532144
		 132 0.23039071695465232 133 0.21306545544270852 134 0.21135424570726286 135 0.22203092272929803
		 136 0.23988092746267212 137 0.25768677773734749 138 0.27407841094672919 139 0.29066278057677725
		 140 0.30776603827690108 141 0.32430258548354929;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_neck_jnt_001_cnt_rotateY";
	rename -uid "BFDD442F-402B-7BE3-AE4A-0B98CDC4DBB4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 -3.6910069688115659 3 -3.3540297822317915
		 4 -2.9299650781028501 5 -2.4283940771186576 6 -1.8593178141255189 7 -1.2327565310940687
		 8 -0.55881020631076461 9 0.15238167776846392 10 0.89069375305708598 11 1.6460661705115629
		 12 2.4085373629039655 13 3.1682432236805802 14 3.9153715790589128 15 4.6396169963233271
		 16 5.330212030035379 17 5.97637523560216 18 6.5674253635555448 19 7.1016016321555737
		 20 7.5840371226438261 21 8.0153709788286136 22 8.3968165778010775 23 8.7297266183852305
		 24 9.0150366201238139 25 9.2520447755897184 26 9.4378764104775108 27 9.5695085275730865
		 28 9.6442278950855922 29 9.6599165744814695 30 9.6151539722223873 31 9.5091592086941965
		 32 9.3416134680643008 33 9.1124141582589644 34 8.8214201763510403 35 8.4682498776297894
		 36 8.0521907352611386 37 7.5772842998415264 38 7.0524734605192183 39 6.4836533817196953
		 40 5.8773123049261642 41 5.2398931962857542 42 4.5779053724966507 43 3.8978775893758377
		 44 3.206312899105237 45 2.5096641901281398 46 1.8143456697326783 47 1.1266679783812206
		 48 0.4533733889356939 49 -0.19811591889448152 50 -0.82034185977340646 51 -1.4063284131768254
		 52 -1.9499532846008762 53 -2.4030673882470097 54 -2.7347063294582963 55 -2.9651514330628888
		 56 -3.1138004555719592 57 -3.1970258977105672 58 -3.2298815002952859 59 -3.2287169703050704
		 60 -3.2101596122970539 61 -3.1912611076282054 62 -3.1895294091664876 63 -3.222863190851708
		 64 -3.3094188200731374 65 -3.4674470447200019 66 -3.7151383795147024 67 -3.9609621188681063
		 68 -4.1031623578813852 69 -4.1445149277198627 70 -4.0877997353833964 71 -3.9357464254578285
		 72 -3.6910069688115659 73 -3.3540297822317915 74 -2.9299650781028501 75 -2.4283940771186576
		 76 -1.8593178141255189 77 -1.2327565310940687 78 -0.55881020631076461 79 0.15238167776846392
		 80 0.89069375305708598 81 1.6460661705115629 82 2.4085373629039655 83 3.1682432236805802
		 84 3.9153715790589128 85 4.6396169963233271 86 5.330212030035379 87 5.97637523560216
		 88 6.5674253635555448 89 7.1016016321555737 90 7.5840371226438261 91 8.0153709788286136
		 92 8.3968165778010775 93 8.7297266183852305 94 9.0150366201238139 95 9.2520447755897184
		 96 9.4378764104775108 97 9.5695085275730865 98 9.6442278950855922 99 9.6599165744814695
		 100 9.6151539722223873 101 9.5091592086941965 102 9.3416134680643008 103 9.1124141582589644
		 104 8.8214201763510403 105 8.4682498776297894 106 8.0521907352611386 107 7.5772842998415264
		 108 7.0524734605192183 109 6.4836533817196953 110 5.8773123049261642 111 5.2398931962857542
		 112 4.5779053724966507 113 3.8978775893758377 114 3.206312899105237 115 2.5096641901281398
		 116 1.8143456697326783 117 1.1266679783812206 118 0.4533733889356939 119 -0.19811591889448152
		 120 -0.82034185977340646 121 -1.4063284131768254 122 -1.9499532846008762 123 -2.4030673882470097
		 124 -2.7347063294582963 125 -2.9651514330628888 126 -3.1138004555719592 127 -3.1970258977105672
		 128 -3.2298815002952859 129 -3.2287169703050704 130 -3.2101596122970539 131 -3.1912611076282054
		 132 -3.1895294091664876 133 -3.222863190851708 134 -3.3094188200731374 135 -3.4674470447200019
		 136 -3.7151383795147024 137 -3.9609621188681063 138 -4.1031623578813852 139 -4.1445149277198627
		 140 -4.0877997353833964 141 -3.9357464254578285;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_neck_jnt_001_cnt_rotateZ";
	rename -uid "E8B6E27E-4F34-0930-A08C-9D990B13B87A";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 21 ".ktv[0:20]"  2 3.4688453999999997 5 3.6386333999999998
		 8 3.0011916000000003 23 -3.0271428000000005 26 -2.7099305999999999 35 2.868462 38 3.5548745999999998
		 41 3.1942092 56 -2.9033219999999997 59 -2.314713 71 3.3978335999999998 74 3.6937362
		 77 3.2687412 92 -2.6536583999999999 95 -3.0454751999999998 107 3.2714621999999998
		 110 3.3762816 125 -2.7527195999999998 128 -2.6297628 140 3.1962977999999995 141 3.3978335999999998;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTU -n "cn_neck_jnt_001_cnt_WorldSpace";
	rename -uid "06C5C7E5-4AF6-6EF0-1183-EBB1E5EFF184";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 1 3 1 4 1 5 1 6 1 7 1 8 1 9 1 10 1 11 1
		 12 1 13 1 14 1 15 1 16 1 17 1 18 1 19 1 20 1 21 1 22 1 23 1 24 1 25 1 26 1 27 1 28 1
		 29 1 30 1 31 1 32 1 33 1 34 1 35 1 36 1 37 1 38 1 39 1 40 1 41 1 42 1 43 1 44 1 45 1
		 46 1 47 1 48 1 49 1 50 1 51 1 52 1 53 1 54 1 55 1 56 1 57 1 58 1 59 1 60 1 61 1 62 1
		 63 1 64 1 65 1 66 1 67 1 68 1 69 1 70 1 71 1 72 1 73 1 74 1 75 1 76 1 77 1 78 1 79 1
		 80 1 81 1 82 1 83 1 84 1 85 1 86 1 87 1 88 1 89 1 90 1 91 1 92 1 93 1 94 1 95 1 96 1
		 97 1 98 1 99 1 100 1 101 1 102 1 103 1 104 1 105 1 106 1 107 1 108 1 109 1 110 1
		 111 1 112 1 113 1 114 1 115 1 116 1 117 1 118 1 119 1 120 1 121 1 122 1 123 1 124 1
		 125 1 126 1 127 1 128 1 129 1 130 1 131 1 132 1 133 1 134 1 135 1 136 1 137 1 138 1
		 139 1 140 1 141 1;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_ft_scapular_001_cnt_translateX";
	rename -uid "CB26B647-46F1-A2CA-EE08-F3BF77B929FD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 4.9430634068480686 3 4.4458358710269721
		 4 3.8906122647800458 5 3.2900694410127898 6 2.6568842526303769 7 2.0037335525382076
		 8 1.343294193641583 9 0.68824302884581812 10 0.051256911056341892 11 -0.55498730682167263
		 12 -1.1178127718827682 13 -1.6245426312216864 14 -2.0625000319330979 15 -2.4190081211116308
		 16 -2.6813900458520408 17 -2.8729857120316069 18 -3.0272564747502457 19 -3.1467076549366482
		 20 -3.2338445735193204 21 -3.2911725514266834 22 -3.3211969095874423 23 -3.3264229689301459
		 24 -3.3093560503832293 25 -3.2725014748753836 26 -3.2183645633350721 27 -3.1494506366908581
		 28 -3.068265015871404 29 -2.9773130218051165 30 -2.8790999754206155 31 -2.7761311976464782
		 32 -2.6709120094112251 33 -2.5659477316434334 34 -2.4637436852715808 35 -2.3668051912243442
		 36 -2.2776375704302296 37 -2.1987461438178144 38 -2.1326362323155195 39 -1.1597830559172024
		 40 -0.12416337445229431 41 0.9552191894331088 42 2.0593610130928255 43 3.1692584738808449
		 44 4.2659079491509715 45 5.3303058162572086 46 6.3434484525533321 47 7.2863322353933029
		 48 8.1399535421310247 49 8.8853087501203731 50 9.5033942367153088 51 9.9659720450380149
		 52 10.271472642157462 53 10.440895041347432 54 10.495238255881517 55 10.455501299033429
		 56 10.342683184077018 57 10.177782924285907 58 9.9817995329338061 59 9.7757320232945091
		 60 9.5805794086416824 61 9.3712617619007119 62 9.1122059240363171 63 8.8086710603815845
		 64 8.4659163362693164 65 8.0892009170323433 66 7.6837839680037376 67 7.2549246545162447
		 68 6.8078821419028515 69 6.3479155954964455 70 5.8802841806298574 71 5.410247062636131
		 72 4.9430634068480686 73 4.4458358710269721 74 3.8906122647800458 75 3.2900694410127898
		 76 2.6568842526303769 77 2.0037335525382076 78 1.343294193641583 79 0.68824302884581812
		 80 0.051256911056341892 81 -0.55498730682167263 82 -1.1178127718827682 83 -1.6245426312216864
		 84 -2.0625000319330979 85 -2.4190081211116308 86 -2.6813900458520408 87 -2.8729857120316069
		 88 -3.0272564747502457 89 -3.1467076549366482 90 -3.2338445735193204 91 -3.2911725514266834
		 92 -3.3211969095874423 93 -3.3264229689301459 94 -3.3093560503832293 95 -3.2725014748753836
		 96 -3.2183645633350721 97 -3.1494506366908581 98 -3.068265015871404 99 -2.9773130218051165
		 100 -2.8790999754206155 101 -2.7761311976464782 102 -2.6709120094112251 103 -2.5659477316434334
		 104 -2.4637436852715808 105 -2.3668051912243442 106 -2.2776375704302296 107 -2.1987461438178144
		 108 -2.1326362323155195 109 -1.1597830559172024 110 -0.12416337445229431 111 0.9552191894331088
		 112 2.0593610130928255 113 3.1692584738808449 114 4.2659079491509715 115 5.3303058162572086
		 116 6.3434484525533321 117 7.2863322353933029 118 8.1399535421310247 119 8.8853087501203731
		 120 9.5033942367153088 121 9.9659720450380149 122 10.271472642157462 123 10.440895041347432
		 124 10.495238255881517 125 10.455501299033429 126 10.342683184077018 127 10.177782924285907
		 128 9.9817995329338061 129 9.7757320232945091 130 9.5805794086416824 131 9.3712617619007119
		 132 9.1122059240363171 133 8.8086710603815845 134 8.4659163362693164 135 8.0892009170323433
		 136 7.6837839680037376 137 7.2549246545162447 138 6.8078821419028515 139 6.3479155954964455
		 140 5.8802841806298574 141 5.410247062636131;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_ft_scapular_001_cnt_translateY";
	rename -uid "DE288DA8-4FB3-E3AD-E391-9483E46490FD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 -10.939824450090498 3 -11.038675002329409
		 4 -11.090907479946988 5 -11.095879829599305 6 -11.052949997942555 7 -10.961475931632819
		 8 -10.820815577326268 9 -10.630326881679022 10 -10.389367791347222 11 -10.097296252986997
		 12 -9.7534702132544702 13 -9.3572476188057934 14 -8.9079864162970956 15 -8.4050445523845028
		 16 -7.8477799737241583 17 -7.2450162474110584 18 -6.6082369743653047 19 -5.9407901519806288
		 20 -5.2460237776508549 21 -4.5272858487697505 22 -3.7879243627310899 23 -3.0312873169286405
		 24 -2.2607227087562052 25 -1.4795785356074944 26 -0.69120279487633951 27 0.10105651604347088
		 28 0.89385139975818362 29 1.6838338588740172 30 2.4676558959972255 31 3.2419695137339701
		 32 4.0034267146905123 33 4.7486795014730774 34 5.4743798766878697 35 6.1771798429411575
		 36 6.8537314028391023 37 7.5006865589879439 38 8.1146973139939362 39 7.7376941880042054
		 40 7.3441959320986214 41 6.932400841692818 42 6.5005072122023861 43 6.0467133390429879
		 44 5.5692175176302214 45 5.0662180433797204 46 4.5359132117070828 47 3.9765013180279638
		 48 3.3861806577579756 49 2.7631495263126737 50 2.10560621910777 51 1.4093221418054043
		 52 0.67707747007687402 53 -0.082416345648582023 54 -0.86044785494171805 55 -1.6483056073732527
		 56 -2.4372781525139615 57 -3.2186540399345489 58 -3.9837218192057975 59 -4.7237700398984472
		 60 -5.4300872515832026 61 -6.1060720818696339 62 -6.7578809009924541 63 -7.3809388724601064
		 64 -7.9706711597811619 65 -8.522502926464071 66 -9.0318593360173232 67 -9.4941655519494432
		 68 -9.9048467377689526 69 -10.259328056984316 70 -10.553034673103994 71 -10.781391749636569
		 72 -10.939824450090498 73 -11.038675002329409 74 -11.090907479946988 75 -11.095879829599305
		 76 -11.052949997942555 77 -10.961475931632819 78 -10.820815577326268 79 -10.630326881679022
		 80 -10.389367791347222 81 -10.097296252986997 82 -9.7534702132544702 83 -9.3572476188057934
		 84 -8.9079864162970956 85 -8.4050445523845028 86 -7.8477799737241583 87 -7.2450162474110584
		 88 -6.6082369743653047 89 -5.9407901519806288 90 -5.2460237776508549 91 -4.5272858487697505
		 92 -3.7879243627310899 93 -3.0312873169286405 94 -2.2607227087562052 95 -1.4795785356074944
		 96 -0.69120279487633951 97 0.10105651604347088 98 0.89385139975818362 99 1.6838338588740172
		 100 2.4676558959972255 101 3.2419695137339701 102 4.0034267146905123 103 4.7486795014730774
		 104 5.4743798766878697 105 6.1771798429411575 106 6.8537314028391023 107 7.5006865589879439
		 108 8.1146973139939362 109 7.7376941880042054 110 7.3441959320986214 111 6.932400841692818
		 112 6.5005072122023861 113 6.0467133390429879 114 5.5692175176302214 115 5.0662180433797204
		 116 4.5359132117070828 117 3.9765013180279638 118 3.3861806577579756 119 2.7631495263126737
		 120 2.10560621910777 121 1.4093221418054043 122 0.67707747007687402 123 -0.082416345648582023
		 124 -0.86044785494171805 125 -1.6483056073732527 126 -2.4372781525139615 127 -3.2186540399345489
		 128 -3.9837218192057975 129 -4.7237700398984472 130 -5.4300872515832026 131 -6.1060720818696339
		 132 -6.7578809009924541 133 -7.3809388724601064 134 -7.9706711597811619 135 -8.522502926464071
		 136 -9.0318593360173232 137 -9.4941655519494432 138 -9.9048467377689526 139 -10.259328056984316
		 140 -10.553034673103994 141 -10.781391749636569;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "rt_ft_scapular_001_cnt_translateZ";
	rename -uid "74440BF2-43A0-D4DB-3002-BDAEFEF9727F";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 -0.63742342256131934 3 -0.47615068188343912
		 4 -0.30077775811579954 5 -0.11526416515849291 6 0.076430583088530568 7 0.27034697272521413
		 8 0.46252548985157205 9 0.64900662056754754 10 0.82583085097314068 11 0.98903866716832312
		 12 1.1346705552530949 13 1.2587670013273922 14 1.3573684914912505 15 1.4265155118446202
		 16 1.4622485484874801 17 1.4724766599630641 18 1.4684442175241728 19 1.4511946763353407
		 20 1.421771491560996 21 1.3812181183656165 22 1.3305780119137083 23 1.2708946273697066
		 24 1.2032114198981034 25 1.1285718446633268 26 1.0480193568298972 27 0.96259741156225687
		 28 0.87334946402492619 29 0.78131896938231193 30 0.68754938279890609 31 0.59308415943919357
		 32 0.49896675446763794 33 0.40624062304873121 34 0.31594922034689432 35 0.22913600152662639
		 36 0.14684442175241941 37 0.070117936188736962 38 1.4210854715202004e-14 39 -0.26933083118954926
		 40 -0.55667250367982035 41 -0.85602140370389179 42 -1.1613739174948563 43 -1.4667264312858208
		 44 -1.7660753313098709 45 -2.0534170038001633 46 -2.3227478349897055 47 -2.568064211111647
		 48 -2.7833625183991018 49 -2.9626391430851697 50 -3.099890471402901 51 -3.1860699071379344
		 52 -3.22091887754749 53 -3.2116158040721814 54 -3.1653391081526792 55 -3.0892672112295614
		 56 -2.9905785347434772 57 -2.8764515001350119 58 -2.7540645288448289 59 -2.6305960423135204
		 60 -2.5132244619817499 61 -2.3939438545587066 62 -2.2612904607887998 63 -2.117255963165448
		 64 -1.9638320441820838 65 -1.8030103863320548 66 -1.6367826721087795 67 -1.4671405840056551
		 68 -1.2960758045160716 69 -1.1255800161334619 70 -0.95764490135119473 71 -0.79426214266267436
		 72 -0.63742342256131934 73 -0.47615068188343912 74 -0.30077775811579954 75 -0.11526416515849291
		 76 0.076430583088530568 77 0.27034697272521413 78 0.46252548985157205 79 0.64900662056754754
		 80 0.82583085097314068 81 0.98903866716832312 82 1.1346705552530949 83 1.2587670013273922
		 84 1.3573684914912505 85 1.4265155118446202 86 1.4622485484874801 87 1.4724766599630641
		 88 1.4684442175241728 89 1.4511946763353407 90 1.421771491560996 91 1.3812181183656165
		 92 1.3305780119137083 93 1.2708946273697066 94 1.2032114198981034 95 1.1285718446633268
		 96 1.0480193568298972 97 0.96259741156225687 98 0.87334946402492619 99 0.78131896938231193
		 100 0.68754938279890609 101 0.59308415943919357 102 0.49896675446763794 103 0.40624062304873121
		 104 0.31594922034689432 105 0.22913600152662639 106 0.14684442175241941 107 0.070117936188736962
		 108 1.4210854715202004e-14 109 -0.26933083118954926 110 -0.55667250367982035 111 -0.85602140370389179
		 112 -1.1613739174948563 113 -1.4667264312858208 114 -1.7660753313098709 115 -2.0534170038001633
		 116 -2.3227478349897055 117 -2.568064211111647 118 -2.7833625183991018 119 -2.9626391430851697
		 120 -3.099890471402901 121 -3.1860699071379344 122 -3.22091887754749 123 -3.2116158040721814
		 124 -3.1653391081526792 125 -3.0892672112295614 126 -2.9905785347434772 127 -2.8764515001350119
		 128 -2.7540645288448289 129 -2.6305960423135204 130 -2.5132244619817499 131 -2.3939438545587066
		 132 -2.2612904607887998 133 -2.117255963165448 134 -1.9638320441820838 135 -1.8030103863320548
		 136 -1.6367826721087795 137 -1.4671405840056551 138 -1.2960758045160716 139 -1.1255800161334619
		 140 -0.95764490135119473 141 -0.79426214266267436;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_ft_scapular_001_cnt_rotateX";
	rename -uid "2DAC881E-486D-9DF5-88CC-539EF41A8E28";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 -1.4312496066585827e-14 3 -1.4312496066585827e-14
		 4 -2.2263882770244608e-14 5 -2.2263882770244617e-14 6 -1.4312496066585827e-14 7 -1.4312496066585827e-14
		 8 -2.2263882770244608e-14 9 -1.4312496066585827e-14 10 -1.4312496066585827e-14 11 -1.4312496066585827e-14
		 12 -2.2263882770244608e-14 13 -1.4312496066585827e-14 14 -1.4312496066585827e-14
		 15 -1.4312496066585827e-14 16 -1.4312496066585827e-14 17 -1.4312496066585827e-14
		 18 -1.4312496066585827e-14 19 -1.4312496066585827e-14 20 -1.4312496066585827e-14
		 21 -1.4312496066585827e-14 22 -1.4312496066585827e-14 23 -1.4312496066585827e-14
		 24 -1.4312496066585827e-14 25 -2.0673605429512861e-14 26 -1.4312496066585827e-14
		 27 -1.4312496066585827e-14 28 -1.4312496066585827e-14 29 -1.4312496066585827e-14
		 30 -2.2263882770244608e-14 31 -2.2263882770244608e-14 32 -1.4312496066585827e-14
		 33 -1.4312496066585827e-14 34 -1.4312496066585827e-14 35 -2.2263882770244608e-14
		 36 -1.4312496066585827e-14 37 -2.2263882770244617e-14 38 -1.9083328088781088e-14
		 39 -1.4312496066585827e-14 40 -1.4312496066585827e-14 41 -1.4312496066585827e-14
		 42 -2.2263882770244617e-14 43 -1.4312496066585827e-14 44 -2.2263882770244617e-14
		 45 -2.2263882770244608e-14 46 -1.4312496066585827e-14 47 -1.4312496066585827e-14
		 48 -1.4312496066585827e-14 49 -1.9083328088781088e-14 50 -1.2722218725854061e-14
		 51 -1.4312496066585827e-14 52 -1.4312496066585827e-14 53 -1.2722218725854061e-14
		 54 -2.2263882770244617e-14 55 -2.2263882770244608e-14 56 -1.4312496066585827e-14
		 57 -1.4312496066585827e-14 58 -1.4312496066585827e-14 59 -1.4312496066585827e-14
		 60 -1.4312496066585827e-14 61 -1.4312496066585827e-14 62 -1.4312496066585827e-14
		 63 -1.4312496066585827e-14 64 -1.4312496066585827e-14 65 -1.4312496066585827e-14
		 66 -6.361109362927032e-15 67 -1.4312496066585827e-14 68 -1.4312496066585827e-14 69 -1.4312496066585827e-14
		 70 -1.4312496066585827e-14 71 -2.2263882770244608e-14 72 -1.4312496066585827e-14
		 73 -1.4312496066585827e-14 74 -2.2263882770244608e-14 75 -2.2263882770244617e-14
		 76 -1.4312496066585827e-14 77 -1.4312496066585827e-14 78 -2.2263882770244608e-14
		 79 -1.4312496066585827e-14 80 -1.4312496066585827e-14 81 -1.4312496066585827e-14
		 82 -2.2263882770244608e-14 83 -1.4312496066585827e-14 84 -1.4312496066585827e-14
		 85 -1.4312496066585827e-14 86 -1.4312496066585827e-14 87 -1.4312496066585827e-14
		 88 -1.4312496066585827e-14 89 -1.4312496066585827e-14 90 -1.4312496066585827e-14
		 91 -1.4312496066585827e-14 92 -1.4312496066585827e-14 93 -1.4312496066585827e-14
		 94 -1.4312496066585827e-14 95 -2.0673605429512861e-14 96 -1.4312496066585827e-14
		 97 -1.4312496066585827e-14 98 -1.4312496066585827e-14 99 -1.4312496066585827e-14
		 100 -2.2263882770244608e-14 101 -2.2263882770244608e-14 102 -1.4312496066585827e-14
		 103 -1.4312496066585827e-14 104 -1.4312496066585827e-14 105 -2.2263882770244608e-14
		 106 -1.4312496066585827e-14 107 -2.2263882770244617e-14 108 -1.9083328088781088e-14
		 109 -1.4312496066585827e-14 110 -1.4312496066585827e-14 111 -1.4312496066585827e-14
		 112 -2.2263882770244617e-14 113 -1.4312496066585827e-14 114 -2.2263882770244617e-14
		 115 -2.2263882770244608e-14 116 -1.4312496066585827e-14 117 -1.4312496066585827e-14
		 118 -1.4312496066585827e-14 119 -1.9083328088781088e-14 120 -1.2722218725854061e-14
		 121 -1.4312496066585827e-14 122 -1.4312496066585827e-14 123 -1.2722218725854061e-14
		 124 -2.2263882770244617e-14 125 -2.2263882770244608e-14 126 -1.4312496066585827e-14
		 127 -1.4312496066585827e-14 128 -1.4312496066585827e-14 129 -1.4312496066585827e-14
		 130 -1.4312496066585827e-14 131 -1.4312496066585827e-14 132 -1.4312496066585827e-14
		 133 -1.4312496066585827e-14 134 -1.4312496066585827e-14 135 -1.4312496066585827e-14
		 136 -6.361109362927032e-15 137 -1.4312496066585827e-14 138 -1.4312496066585827e-14
		 139 -1.4312496066585827e-14 140 -1.4312496066585827e-14 141 -2.2263882770244608e-14;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_ft_scapular_001_cnt_rotateY";
	rename -uid "86D86677-4D15-A722-F0D8-C9BC2BCFE598";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 1.5902773407317581e-14 3 1.5902773407317587e-14
		 4 4.2937488199757475e-14 5 4.2937488199757475e-14 6 1.5902773407317584e-14 7 1.5902773407317584e-14
		 8 4.2937488199757481e-14 9 1.590277340731759e-14 10 1.5902773407317587e-14 11 1.5902773407317587e-14
		 12 4.2937488199757475e-14 13 1.5902773407317584e-14 14 1.5902773407317584e-14 15 1.5902773407317584e-14
		 16 1.5902773407317587e-14 17 1.5902773407317584e-14 18 1.5902773407317581e-14 19 1.5902773407317584e-14
		 20 1.5902773407317584e-14 21 1.5902773407317587e-14 22 1.5902773407317584e-14 23 1.5902773407317584e-14
		 24 1.5902773407317587e-14 25 4.4527765540489235e-14 26 1.5902773407317584e-14 27 1.5902773407317587e-14
		 28 1.5902773407317584e-14 29 1.5902773407317587e-14 30 4.2937488199757475e-14 31 4.2937488199757481e-14
		 32 1.590277340731759e-14 33 1.5902773407317584e-14 34 1.5902773407317587e-14 35 4.2937488199757475e-14
		 36 1.5902773407317584e-14 37 4.2937488199757475e-14 38 4.1347210859025721e-14 39 1.590277340731759e-14
		 40 1.5902773407317584e-14 41 1.5902773407317587e-14 42 4.2937488199757475e-14 43 1.5902773407317584e-14
		 44 4.2937488199757475e-14 45 4.2937488199757481e-14 46 1.590277340731759e-14 47 1.5902773407317587e-14
		 48 1.5902773407317587e-14 49 4.1347210859025721e-14 50 1.4312496066585827e-14 51 1.5902773407317587e-14
		 52 1.5902773407317587e-14 53 1.4312496066585827e-14 54 4.2937488199757475e-14 55 4.2937488199757481e-14
		 56 1.590277340731759e-14 57 1.5902773407317587e-14 58 1.5902773407317587e-14 59 1.5902773407317581e-14
		 60 1.5902773407317584e-14 61 1.5902773407317584e-14 62 1.5902773407317587e-14 63 1.5902773407317587e-14
		 64 1.5902773407317587e-14 65 1.5902773407317584e-14 66 -9.5416640443905503e-15 67 1.5902773407317584e-14
		 68 1.5902773407317584e-14 69 1.5902773407317587e-14 70 1.5902773407317587e-14 71 4.2937488199757475e-14
		 72 1.5902773407317584e-14 73 1.5902773407317587e-14 74 4.2937488199757475e-14 75 4.2937488199757475e-14
		 76 1.5902773407317584e-14 77 1.5902773407317584e-14 78 4.2937488199757481e-14 79 1.590277340731759e-14
		 80 1.5902773407317587e-14 81 1.5902773407317587e-14 82 4.2937488199757475e-14 83 1.5902773407317584e-14
		 84 1.5902773407317584e-14 85 1.5902773407317584e-14 86 1.5902773407317587e-14 87 1.5902773407317584e-14
		 88 1.5902773407317581e-14 89 1.5902773407317584e-14 90 1.5902773407317584e-14 91 1.5902773407317587e-14
		 92 1.5902773407317584e-14 93 1.5902773407317584e-14 94 1.5902773407317587e-14 95 4.4527765540489235e-14
		 96 1.5902773407317584e-14 97 1.5902773407317587e-14 98 1.5902773407317584e-14 99 1.5902773407317587e-14
		 100 4.2937488199757475e-14 101 4.2937488199757481e-14 102 1.590277340731759e-14 103 1.5902773407317584e-14
		 104 1.5902773407317587e-14 105 4.2937488199757475e-14 106 1.5902773407317584e-14
		 107 4.2937488199757475e-14 108 4.1347210859025721e-14 109 1.590277340731759e-14 110 1.5902773407317584e-14
		 111 1.5902773407317587e-14 112 4.2937488199757475e-14 113 1.5902773407317584e-14
		 114 4.2937488199757475e-14 115 4.2937488199757481e-14 116 1.590277340731759e-14 117 1.5902773407317587e-14
		 118 1.5902773407317587e-14 119 4.1347210859025721e-14 120 1.4312496066585827e-14
		 121 1.5902773407317587e-14 122 1.5902773407317587e-14 123 1.4312496066585827e-14
		 124 4.2937488199757475e-14 125 4.2937488199757481e-14 126 1.590277340731759e-14 127 1.5902773407317587e-14
		 128 1.5902773407317587e-14 129 1.5902773407317581e-14 130 1.5902773407317584e-14
		 131 1.5902773407317584e-14 132 1.5902773407317587e-14 133 1.5902773407317587e-14
		 134 1.5902773407317587e-14 135 1.5902773407317584e-14 136 -9.5416640443905503e-15
		 137 1.5902773407317584e-14 138 1.5902773407317584e-14 139 1.5902773407317587e-14
		 140 1.5902773407317587e-14 141 4.2937488199757475e-14;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "rt_ft_scapular_001_cnt_rotateZ";
	rename -uid "7F60B432-42C1-D673-5427-EDAB8F8FDEA2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 140 ".ktv[0:139]"  2 -1.272221872585407e-14 3 1.2722218725854064e-14
		 4 1.9083328088781091e-14 5 -8.4747000922563045e-30 6 1.2722218725854064e-14 7 -1.4124500153760508e-30
		 8 1.9083328088781094e-14 9 1.2722218725854065e-14 10 1.2722218725854065e-14 11 1.2722218725854065e-14
		 12 1.9083328088781091e-14 13 1.2722218725854065e-14 14 -1.4124500153760508e-30 15 -1.9862578341225712e-30
		 16 1.2722218725854065e-14 17 -1.4124500153760508e-30 18 -1.2722218725854067e-14 19 -1.4124500153760508e-30
		 20 -1.9862578341225712e-30 21 1.2722218725854065e-14 22 -1.4124500153760508e-30 23 -1.9862578341225712e-30
		 24 1.2722218725854065e-14 25 -6.3611093629270406e-15 26 -1.4124500153760508e-30 27 1.2722218725854065e-14
		 28 -1.4124500153760508e-30 29 1.2722218725854065e-14 30 1.9083328088781091e-14 31 1.9083328088781091e-14
		 32 1.2722218725854065e-14 33 -1.4124500153760508e-30 34 1.2722218725854065e-14 35 1.9083328088781091e-14
		 36 1.2722218725854065e-14 37 -7.0622500768802538e-30 38 3.1805546814635161e-14 39 1.2722218725854067e-14
		 40 -2.8249000307521015e-30 41 1.2722218725854064e-14 42 -8.4747000922563045e-30 43 1.2722218725854064e-14
		 44 -8.4747000922563045e-30 45 1.9083328088781091e-14 46 1.2722218725854065e-14 47 1.2722218725854065e-14
		 48 1.2722218725854065e-14 49 3.1805546814635161e-14 50 3.8166656177562195e-14 51 1.2722218725854067e-14
		 52 1.2722218725854065e-14 53 3.8166656177562195e-14 54 -5.6498000615042044e-30 55 1.9083328088781091e-14
		 56 1.2722218725854065e-14 57 1.2722218725854065e-14 58 1.2722218725854065e-14 59 -1.2722218725854067e-14
		 60 -1.4124500153760508e-30 61 -1.9862578341225712e-30 62 1.2722218725854065e-14 63 1.2722218725854065e-14
		 64 1.2722218725854065e-14 65 -1.4124500153760508e-30 66 -1.2722218725854067e-14 67 -1.4124500153760508e-30
		 68 -1.9862578341225712e-30 69 1.2722218725854065e-14 70 1.2722218725854065e-14 71 1.9083328088781091e-14
		 72 -1.272221872585407e-14 73 1.2722218725854064e-14 74 1.9083328088781091e-14 75 -8.4747000922563045e-30
		 76 1.2722218725854064e-14 77 -1.4124500153760508e-30 78 1.9083328088781094e-14 79 1.2722218725854065e-14
		 80 1.2722218725854065e-14 81 1.2722218725854065e-14 82 1.9083328088781091e-14 83 1.2722218725854065e-14
		 84 -1.4124500153760508e-30 85 -1.9862578341225712e-30 86 1.2722218725854065e-14 87 -1.4124500153760508e-30
		 88 -1.2722218725854067e-14 89 -1.4124500153760508e-30 90 -1.9862578341225712e-30
		 91 1.2722218725854065e-14 92 -1.4124500153760508e-30 93 -1.9862578341225712e-30 94 1.2722218725854065e-14
		 95 -6.3611093629270406e-15 96 -1.4124500153760508e-30 97 1.2722218725854065e-14 98 -1.4124500153760508e-30
		 99 1.2722218725854065e-14 100 1.9083328088781091e-14 101 1.9083328088781091e-14 102 1.2722218725854065e-14
		 103 -1.4124500153760508e-30 104 1.2722218725854065e-14 105 1.9083328088781091e-14
		 106 1.2722218725854065e-14 107 -7.0622500768802538e-30 108 3.1805546814635161e-14
		 109 1.2722218725854067e-14 110 -2.8249000307521015e-30 111 1.2722218725854064e-14
		 112 -8.4747000922563045e-30 113 1.2722218725854064e-14 114 -8.4747000922563045e-30
		 115 1.9083328088781091e-14 116 1.2722218725854065e-14 117 1.2722218725854065e-14
		 118 1.2722218725854065e-14 119 3.1805546814635161e-14 120 3.8166656177562195e-14
		 121 1.2722218725854067e-14 122 1.2722218725854065e-14 123 3.8166656177562195e-14
		 124 -5.6498000615042044e-30 125 1.9083328088781091e-14 126 1.2722218725854065e-14
		 127 1.2722218725854065e-14 128 1.2722218725854065e-14 129 -1.2722218725854067e-14
		 130 -1.4124500153760508e-30 131 -1.9862578341225712e-30 132 1.2722218725854065e-14
		 133 1.2722218725854065e-14 134 1.2722218725854065e-14 135 -1.4124500153760508e-30
		 136 -1.2722218725854067e-14 137 -1.4124500153760508e-30 138 -1.9862578341225712e-30
		 139 1.2722218725854065e-14 140 1.2722218725854065e-14 141 1.9083328088781091e-14;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_ft_scapular_001_cnt_translateX";
	rename -uid "A4559E2B-40A5-B538-0F9C-1689E2BE4937";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 12 ".ktv[0:11]"  2 -2.1326362669552594 5 0.95521915984032546
		 8 4.2659079253772916 11 7.2863322184185648 14 9.5033942277269858 17 10.440895041699235
		 20 10.342683194481182 26 9.1122059528841675 29 8.0892009534128846 32 6.8078821841842938
		 41 12.201062674395136 72 -2.1326362669552594;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_ft_scapular_001_cnt_translateY";
	rename -uid "B0A274E9-4304-33C5-34AD-25AB6F3A5DB7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 13 ".ktv[0:12]"  2 8.114697304890214 5 6.9324008457703883
		 8 5.5692175358403588 11 3.9765013491315599 14 2.1056062596755112 17 -0.08241630107890785
		 23 -4.7237699981681445 26 -6.757880862094602 29 -8.5225028919331933 32 -9.9048467087077334
		 35 -10.781391726541518 41 -7.393408894700797 72 8.114697304890214;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_ft_scapular_001_cnt_translateZ";
	rename -uid "C1F9927F-4561-F7EC-B556-9CB82604C9D7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 12 ".ktv[0:11]"  2 0 5 0.8560214037038989 8 1.7660753313098994
		 11 2.5680642111116896 14 3.0998904714029507 17 3.2116158040722169 20 2.9905785347434914
		 26 2.2612904607887998 29 1.8030103863320761 35 0.79426214266266726 41 3.1427623222160816
		 72 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_ft_scapular_001_cnt_rotateX";
	rename -uid "AB5483A2-41B1-3CFB-8CB9-998FEA3CB9DE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 12 ".ktv[0:11]"  2 1.4312496066585827e-14 5 7.951386703658803e-15
		 8 1.4312496066585827e-14 11 1.2722218725854067e-14 14 7.9513867036587967e-15 17 1.4312496066585827e-14
		 20 7.9513867036587982e-15 23 1.2722218725854067e-14 29 7.951386703658803e-15 32 1.4312496066585827e-14
		 35 1.2722218725854067e-14 72 1.4312496066585827e-14;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_ft_scapular_001_cnt_rotateY";
	rename -uid "15F48A4F-45DE-485E-7698-598490F18E8A";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 13 ".ktv[0:12]"  2 -5.5615219355432012e-30 5 2.5444437451708122e-14
		 8 -5.6498000615042044e-30 11 -1.5902773407317641e-15 14 2.7034714792439888e-14 17 -5.6498000615042044e-30
		 20 2.7034714792439888e-14 23 -1.5902773407317619e-15 26 -1.5902773407317641e-15 29 2.5444437451708122e-14
		 32 -5.6498000615042044e-30 35 1.5902773407317527e-15 72 -5.5615219355432012e-30;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_ft_scapular_001_cnt_rotateZ";
	rename -uid "F24D8751-4318-5647-D116-C7AB1D0FC8F6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 12 ".ktv[0:11]"  2 4.4527765540489241e-14 5 6.361109362927031e-14
		 8 4.4527765540489241e-14 11 3.1805546814635168e-14 17 4.4527765540489241e-14 20 3.8166656177562195e-14
		 23 3.1805546814635168e-14 26 3.1805546814635168e-14 29 6.361109362927031e-14 32 4.4527765540489241e-14
		 35 6.3611093629270335e-14 72 4.4527765540489241e-14;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode script -n "uiConfigurationScriptNode";
	rename -uid "0DBA9578-4841-EBDA-0B23-FDAB77250B25";
	setAttr ".b" -type "string" (
		"// Maya Mel UI Configuration File.\n//\n//  This script is machine generated.  Edit at your own risk.\n//\n//\n\nglobal string $gMainPane;\nif (`paneLayout -exists $gMainPane`) {\n\n\tglobal int $gUseScenePanelConfig;\n\tint    $useSceneConfig = $gUseScenePanelConfig;\n\tint    $nodeEditorPanelVisible = stringArrayContains(\"nodeEditorPanel1\", `getPanel -vis`);\n\tint    $nodeEditorWorkspaceControlOpen = (`workspaceControl -exists nodeEditorPanel1Window` && `workspaceControl -q -visible nodeEditorPanel1Window`);\n\tint    $menusOkayInPanels = `optionVar -q allowMenusInPanels`;\n\tint    $nVisPanes = `paneLayout -q -nvp $gMainPane`;\n\tint    $nPanes = 0;\n\tstring $editorName;\n\tstring $panelName;\n\tstring $itemFilterName;\n\tstring $panelConfig;\n\n\t//\n\t//  get current state of the UI\n\t//\n\tsceneUIReplacement -update $gMainPane;\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Top View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Top View\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        modelEditor -e \n            -docTag \"RADRENDER\" \n            -editorChanged \"updateModelPanelBar\" \n            -camera \"top\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 32768\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n"
		+ "            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n"
		+ "            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n"
		+ "            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Side View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Side View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -docTag \"RADRENDER\" \n            -editorChanged \"updateModelPanelBar\" \n            -camera \"side\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n"
		+ "            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 32768\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n"
		+ "            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n"
		+ "            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Front View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Front View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -docTag \"RADRENDER\" \n            -editorChanged \"updateModelPanelBar\" \n            -camera \"front\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n"
		+ "            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 32768\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n"
		+ "            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n"
		+ "            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Persp View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Persp View\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        modelEditor -e \n            -docTag \"RADRENDER\" \n            -editorChanged \"updateModelPanelBar\" \n            -camera \"persp\" \n            -useInteractiveMode 0\n            -displayLights \"all\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 1\n            -activeComponentsXray 0\n            -displayTextures 1\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 32768\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n"
		+ "            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n"
		+ "            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 0\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 1\n            -captureSequenceNumber -1\n            -width 1319\n            -height 708\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n"
		+ "            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"Outliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"Outliner\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -docTag \"isolOutln_fromSeln\" \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n            -showReferenceNodes 0\n            -showReferenceMembers 0\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 1\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n            -showParentContainers 0\n"
		+ "            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n            -isSet 0\n            -isSetMember 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n"
		+ "            -sortOrder \"none\" \n            -longNames 0\n            -niceNames 1\n            -selectCommand \"print(\\\"\\\")\" \n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            -renderFilterIndex 0\n            -selectionOrder \"chronological\" \n            -expandAttribute 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"Outliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"Outliner\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -docTag \"isolOutln_fromSeln\" \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n            -showReferenceNodes 0\n            -showReferenceMembers 0\n"
		+ "            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 1\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n            -showParentContainers 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n"
		+ "            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n            -longNames 0\n            -niceNames 1\n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"graphEditor\" (localizedPanelLabel(\"Graph Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Graph Editor\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showAssignedMaterials 0\n                -showTimeEditor 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -organizeByClip 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 1\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showParentContainers 0\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 1\n                -showCompounds 0\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n"
		+ "                -highlightActive 0\n                -autoSelectNewObjects 1\n                -doNotSelectNewObjects 0\n                -dropIsParent 1\n                -transmitFilters 1\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 1\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n"
		+ "                -renderFilterVisible 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"GraphEd\");\n            animCurveEditor -e \n                -displayValues 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n                -showPlayRangeShades \"on\" \n                -lockPlayRangeShades \"off\" \n                -smoothness \"fine\" \n                -resultSamples 1\n                -resultScreenSamples 0\n                -resultUpdate \"delayed\" \n                -showUpstreamCurves 1\n                -keyMinScale 1\n                -stackedCurvesMin -1\n                -stackedCurvesMax 1\n                -stackedCurvesSpace 0.2\n                -preSelectionHighlight 0\n                -constrainDrag 0\n                -valueLinesToggle 1\n                -outliner \"graphEditor1OutlineEd\" \n                -highlightAffectedCurves 0\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dopeSheetPanel\" (localizedPanelLabel(\"Dope Sheet\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dope Sheet\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showAssignedMaterials 0\n                -showTimeEditor 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -organizeByClip 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 0\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showParentContainers 1\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n"
		+ "                -showUpstreamCurves 1\n                -showUnitlessCurves 0\n                -showCompounds 1\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 0\n                -doNotSelectNewObjects 1\n                -dropIsParent 1\n                -transmitFilters 0\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n"
		+ "                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 0\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                -renderFilterVisible 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"DopeSheetEd\");\n            dopeSheetEditor -e \n                -displayValues 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n                -outliner \"dopeSheetPanel1OutlineEd\" \n                -showSummary 1\n                -showScene 0\n                -hierarchyBelow 0\n                -showTicks 1\n                -selectionWindow 0 0 0 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"timeEditorPanel\" (localizedPanelLabel(\"Time Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Time Editor\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"clipEditorPanel\" (localizedPanelLabel(\"Trax Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Trax Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = clipEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayValues 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -initialized 0\n                -manageSequencer 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"sequenceEditorPanel\" (localizedPanelLabel(\"Camera Sequencer\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Camera Sequencer\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = sequenceEditorNameFromPanel($panelName);\n"
		+ "            clipEditor -e \n                -displayValues 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -initialized 0\n                -manageSequencer 1 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperGraphPanel\" (localizedPanelLabel(\"Hypergraph Hierarchy\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypergraph Hierarchy\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"HyperGraphEd\");\n            hyperGraph -e \n                -graphLayoutStyle \"hierarchicalLayout\" \n                -orientation \"horiz\" \n                -mergeConnections 0\n                -zoom 1\n                -animateTransition 0\n                -showRelationships 1\n                -showShapes 0\n                -showDeformers 0\n                -showExpressions 0\n                -showConstraints 0\n"
		+ "                -showConnectionFromSelected 0\n                -showConnectionToSelected 0\n                -showConstraintLabels 0\n                -showUnderworld 0\n                -showInvisible 0\n                -transitionFrames 1\n                -opaqueContainers 0\n                -freeform 0\n                -image \"T:/Maya_Tak/data/TK_Test_1032_Sc007_v01_DQ_retake_01.ma_TK_Tak_hairSystem_Tak__HairShape.mchp\" \n                -imagePosition 0 0 \n                -imageScale 1\n                -imageEnabled 0\n                -graphType \"DAG\" \n                -heatMapDisplay 0\n                -updateSelection 1\n                -updateNodeAdded 1\n                -useDrawOverrideColor 0\n                -limitGraphTraversal -1\n                -range 0 0 \n                -iconSize \"smallIcons\" \n                -showCachedConnections 0\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperShadePanel\" (localizedPanelLabel(\"Hypershade\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypershade\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"visorPanel\" (localizedPanelLabel(\"Visor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Visor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"nodeEditorPanel\" (localizedPanelLabel(\"Node Editor\")) `;\n\tif ($nodeEditorPanelVisible || $nodeEditorWorkspaceControlOpen) {\n\t\tif (\"\" == $panelName) {\n\t\t\tif ($useSceneConfig) {\n\t\t\t\t$panelName = `scriptedPanel -unParent  -type \"nodeEditorPanel\" -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels `;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n"
		+ "                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -connectNodeOnCreation 0\n                -connectOnDrop 0\n                -copyConnectionsOnPaste 0\n                -connectionStyle \"bezier\" \n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -enableOpenGL 0\n                -gridSnap 0\n                -gridVisibility 1\n                -crosshairOnEdgeDragging 0\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -editorMode \"default\" \n"
		+ "                -hasWatchpoint 0\n                $editorName;\n\t\t\t}\n\t\t} else {\n\t\t\t$label = `panel -q -label $panelName`;\n\t\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -connectNodeOnCreation 0\n                -connectOnDrop 0\n                -copyConnectionsOnPaste 0\n                -connectionStyle \"bezier\" \n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -enableOpenGL 0\n                -gridSnap 0\n                -gridVisibility 1\n                -crosshairOnEdgeDragging 0\n"
		+ "                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -editorMode \"default\" \n                -hasWatchpoint 0\n                $editorName;\n\t\t\tif (!$useSceneConfig) {\n\t\t\t\tpanel -e -l $label $panelName;\n\t\t\t}\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"createNodePanel\" (localizedPanelLabel(\"Create Node\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Create Node\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"polyTexturePlacementPanel\" (localizedPanelLabel(\"UV Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"UV Editor\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"renderWindowPanel\" (localizedPanelLabel(\"Render View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Render View\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"shapePanel\" (localizedPanelLabel(\"Shape Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tshapePanel -edit -l (localizedPanelLabel(\"Shape Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"posePanel\" (localizedPanelLabel(\"Pose Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tposePanel -edit -l (localizedPanelLabel(\"Pose Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n"
		+ "\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynRelEdPanel\" (localizedPanelLabel(\"Dynamic Relationships\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dynamic Relationships\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"relationshipPanel\" (localizedPanelLabel(\"Relationship Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Relationship Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"referenceEditorPanel\" (localizedPanelLabel(\"Reference Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Reference Editor\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"componentEditorPanel\" (localizedPanelLabel(\"Component Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Component Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynPaintScriptedPanelType\" (localizedPanelLabel(\"Paint Effects\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Paint Effects\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"scriptEditorPanel\" (localizedPanelLabel(\"Script Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Script Editor\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"profilerPanel\" (localizedPanelLabel(\"Profiler Tool\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Profiler Tool\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"contentBrowserPanel\" (localizedPanelLabel(\"Content Browser\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Content Browser\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"Stereo\" (localizedPanelLabel(\"Stereo\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Stereo\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "{ string $editorName = ($panelName+\"Editor\");\n            stereoCameraView -e \n                -editorChanged \"updateModelPanelBar\" \n                -camera \"persp\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"smoothShaded\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 0\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 32768\n"
		+ "                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -rendererOverrideName \"stereoOverrideVP2\" \n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 4 4 \n                -bumpResolution 4 4 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 0\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n"
		+ "                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -controllers 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n"
		+ "                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 0\n                -height 0\n                -sceneRenderFilter 0\n                -displayMode \"centerEye\" \n                -viewColor 0 0 0 1 \n                -useCustomBackground 1\n                $editorName;\n            stereoCameraView -e -viewSelected 0 $editorName;\n            stereoCameraView -e \n                -pluginObjects \"gpuCacheDisplayFilter\" 1 \n                $editorName; };\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"ToggledOutliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"ToggledOutliner\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n"
		+ "            -showReferenceNodes 1\n            -showReferenceMembers 1\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 1\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n            -showParentContainers 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n"
		+ "            -alwaysToggleSelect 0\n            -directSelect 0\n            -isSet 0\n            -isSetMember 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n            -longNames 0\n            -niceNames 1\n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            -renderFilterIndex 0\n            -selectionOrder \"chronological\" \n            -expandAttribute 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\tif ($useSceneConfig) {\n        string $configName = `getPanel -cwl (localizedPanelLabel(\"Current Layout\"))`;\n"
		+ "        if (\"\" != $configName) {\n\t\t\tpanelConfiguration -edit -label (localizedPanelLabel(\"Current Layout\")) \n\t\t\t\t-userCreated false\n\t\t\t\t-defaultImage \"vacantCell.xP:/\"\n\t\t\t\t-image \"\"\n\t\t\t\t-sc false\n\t\t\t\t-configString \"global string $gMainPane; paneLayout -e -cn \\\"single\\\" -ps 1 100 100 $gMainPane;\"\n\t\t\t\t-removeAllPanels\n\t\t\t\t-ap false\n\t\t\t\t\t(localizedPanelLabel(\"Persp View\")) \n\t\t\t\t\t\"modelPanel\"\n"
		+ "\t\t\t\t\t\"$panelName = `modelPanel -unParent -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels `;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -docTag \\\"RADRENDER\\\" \\n    -editorChanged \\\"updateModelPanelBar\\\" \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"all\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 1\\n    -activeComponentsXray 0\\n    -displayTextures 1\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 32768\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"vp2Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -controllers 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 0\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 1\\n    -captureSequenceNumber -1\\n    -width 1319\\n    -height 708\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName;\\nmodelEditor -e \\n    -pluginObjects \\\"gpuCacheDisplayFilter\\\" 1 \\n    $editorName\"\n"
		+ "\t\t\t\t\t\"modelPanel -edit -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels  $panelName;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -docTag \\\"RADRENDER\\\" \\n    -editorChanged \\\"updateModelPanelBar\\\" \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"all\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 1\\n    -activeComponentsXray 0\\n    -displayTextures 1\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 32768\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"vp2Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -controllers 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 0\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 1\\n    -captureSequenceNumber -1\\n    -width 1319\\n    -height 708\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName;\\nmodelEditor -e \\n    -pluginObjects \\\"gpuCacheDisplayFilter\\\" 1 \\n    $editorName\"\n"
		+ "\t\t\t\t$configName;\n\n            setNamedPanelLayout (localizedPanelLabel(\"Current Layout\"));\n        }\n\n        panelHistory -e -clear mainPanelHistory;\n        sceneUIReplacement -clear;\n\t}\n\n\ngrid -spacing 5 -size 12 -divisions 5 -displayAxes yes -displayGridLines yes -displayDivisionLines yes -displayPerspectiveLabels no -displayOrthographicLabels no -displayAxesBold yes -perspectiveLabelPosition axis -orthographicLabelPosition edge;\nviewManip -drawCompass 0 -compassAngle 0 -frontParameters \"1 -0.624695 -0.468521 -0.624695 -0.331295 0.883452 -0.331295\" -homeParameters \"\" -selectionLockParameters \"\";\n}\n");
	setAttr ".st" 3;
createNode script -n "sceneConfigurationScriptNode";
	rename -uid "1D913EB1-4A3B-B778-F598-6BAB86C8D55A";
	setAttr ".b" -type "string" "playbackOptions -min 2 -max 300 -ast 1 -aet 300 ";
	setAttr ".st" 6;
createNode animCurveTA -n "cn_jaw_jnt_001_cnt_rotateX";
	rename -uid "63B04B5C-4DA0-7A44-6F5D-2491DEF9DBD4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  19 0 76 -1.8441663409103117 110 0;
	setAttr -s 3 ".kit[2]"  1;
	setAttr -s 3 ".kot[2]"  1;
	setAttr -s 3 ".kix[2]"  1;
	setAttr -s 3 ".kiy[2]"  0;
	setAttr -s 3 ".kox[2]"  1;
	setAttr -s 3 ".koy[2]"  0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_jaw_jnt_001_cnt_rotateY";
	rename -uid "CB2F40C9-46C0-FB9C-C4A9-B69FB2A98FBC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  19 0 76 3.7798333377307665 110 0;
	setAttr -s 3 ".kit[2]"  1;
	setAttr -s 3 ".kot[2]"  1;
	setAttr -s 3 ".kix[2]"  1;
	setAttr -s 3 ".kiy[2]"  0;
	setAttr -s 3 ".kox[2]"  1;
	setAttr -s 3 ".koy[2]"  0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "cn_jaw_jnt_001_cnt_rotateZ";
	rename -uid "39B2ACF4-46F8-47DE-62EA-C48E1ADE7FFD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  19 0 76 -5.6964890509148614 110 0;
	setAttr -s 3 ".kit[2]"  1;
	setAttr -s 3 ".kot[2]"  1;
	setAttr -s 3 ".kix[2]"  1;
	setAttr -s 3 ".kiy[2]"  0;
	setAttr -s 3 ".kox[2]"  1;
	setAttr -s 3 ".koy[2]"  0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_jaw_jnt_001_cnt_translateX";
	rename -uid "8CA41D11-41D2-1F4D-5FD0-228B73B9DF61";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  19 0 110 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_jaw_jnt_001_cnt_translateY";
	rename -uid "244FCA50-432C-9300-8568-AF9784D1DA91";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  19 0 110 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "cn_jaw_jnt_001_cnt_translateZ";
	rename -uid "28016EEE-4461-2E50-48EA-2EB781880ECE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  19 0 110 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_bk_foot_001_cnt_rotateZ";
	rename -uid "CA2A30C1-4CAF-93E2-5FA6-93A16BC5E8DE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  17 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_bk_foot_001_cnt_rotateY";
	rename -uid "512A51DA-46DD-B67C-9BD2-99A97210F698";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  17 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTA -n "lf_bk_foot_001_cnt_rotateX";
	rename -uid "00A384AA-4F29-B512-22AC-CAAE312EF977";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 34 ".ktv[0:33]"  17 -22.164043083503074 25 0 43 1.4581034514543101
		 46 7.8704447663726782 49 16.967021980559196 52 26.063599194745716 55 32.475940509664078
		 58 33.751205751491483 61 31.208092472135302 64 26.196663362815798 67 19.390095468068395
		 70 11.461565832428555 76 -5.0686704833865566 79 -12.324023074490931 82 -18.008629228345917
		 85 -21.449311900416024 88 -20.522262114354699 91 -5.7462333920193132 94 0 112 0.38109522026646747
		 115 5.3021943689247504 118 13.802274716607238 121 23.197100364045763 124 30.802435411972194
		 127 33.934043961118384 130 32.363297523868987 133 28.099842908477754 136 21.816857159480154
		 139 14.187517321411635 145 -2.4175164437963144 148 -10.046856281864835 151 -16.32984203086243
		 154 -20.59329664625367 156 -21.981204873876134;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_bk_foot_001_cnt_translateZ";
	rename -uid "44F8EE22-4E19-531E-1F67-F08B80A7B445";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 39 ".ktv[0:38]"  17 19.926541910624465 20 21.887695979845802
		 25 19.021076406271774 28 11.457627049104673 31 2.7572950684317732 34 -4.7693436087687218
		 37 -10.111332574100174 40 -13.835379449916246 43 -16.006822512295827 46 -16.875516312158695
		 49 -16.872251576444754 55 -16.083060256611986 58 -15.864642279914847 61 -14.204886596939645
		 64 -10.963067656783551 67 -6.6091779518113043 70 -1.5839755367951796 76 8.873674622099287
		 79 13.516587828470946 82 17.200565535413915 85 19.455461000817124 91 23.742843815024514
		 94 20.890226806799092 97 14.219199641240181 103 -2.5335025603950214 106 -8.5132024163011408
		 109 -12.770349975383752 112 -15.447010931512651 115 -16.705506770923705 118 -16.941014519917857
		 130 -14.957503100938879 133 -12.190041584385071 136 -8.1560539204760545 139 -3.3079732042898513
		 145 7.1892166978948353 148 12.053200190886656 151 16.107580409583832 154 18.89250406011702
		 156 19.805941487864672;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_bk_foot_001_cnt_translateY";
	rename -uid "C8A67931-49C9-1BB6-9D59-88997646B6D3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 34 ".ktv[0:33]"  17 3.0069622547181414 20 1.7285271366930139
		 25 0.89198791953621548 43 1.2935010861315162 46 2.06986392485023 49 4.1640576086332377
		 52 5.9432625498680149 55 7.014672721238794 58 7.2432516743900042 64 7.3585154771670194
		 67 7.2840096120284219 70 6.976219805564158 73 6.387749852220173 76 5.5552790307019055
		 82 3.7225200268834993 85 3.1342516190545275 91 0.95410361499540564 94 0.89198791953621548
		 112 0.9977180853812504 115 1.4207499335363565 118 3.471612111509506 121 5.4159865725246537
		 124 6.7492198942045167 127 7.237964228616276 130 7.2796015825369462 133 7.3487473942131469
		 136 7.3304107241586198 139 7.1092245081524279 142 6.6147061845557875 145 5.8538801381695311
		 148 4.9229855074773159 151 3.9945708109885221 154 3.2844172855116422 156 3.0396892281142094;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
createNode animCurveTL -n "lf_bk_foot_001_cnt_translateX";
	rename -uid "35D64B97-4D2C-5F0D-EB34-19A9B21FF54E";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 2 ".ktv[0:1]"  17 0 156 0;
	setAttr ".pre" 4;
	setAttr ".pst" 4;
select -ne :time1;
	setAttr -av -k on ".cch";
	setAttr -av -k on ".ihi";
	setAttr -av -cb on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k on ".o" 2;
	setAttr -av -k on ".unw" 2;
	setAttr -av -k on ".etw";
	setAttr -av -k on ".tps";
	setAttr -av -k on ".tms";
select -ne :hardwareRenderingGlobals;
	setAttr -av -k on ".ihi";
	setAttr ".otfna" -type "stringArray" 22 "NURBS Curves" "NURBS Surfaces" "Polygons" "Subdiv Surface" "Particles" "Particle Instance" "Fluids" "Strokes" "Image Planes" "UI" "Lights" "Cameras" "Locators" "Joints" "IK Handles" "Deformers" "Motion Trails" "Components" "Hair Systems" "Follicles" "Misc. UI" "Ornaments"  ;
	setAttr ".otfva" -type "Int32Array" 22 0 1 1 1 1 1
		 1 1 1 0 0 0 0 0 0 0 0 0
		 0 0 0 0 ;
	setAttr -av ".ta";
	setAttr -av ".tq";
	setAttr ".aoon" yes;
	setAttr -av ".aoam";
	setAttr -av ".aora";
	setAttr -av ".hfs";
	setAttr -av ".hfe";
	setAttr -av ".mbe";
	setAttr -av -k on ".mbsof";
	setAttr ".msaa" yes;
	setAttr ".fprt" yes;
select -ne :renderPartition;
	setAttr -av -k on ".cch";
	setAttr -k on ".ihi";
	setAttr -av -cb on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 8 ".st";
	setAttr -k on ".an";
	setAttr -k on ".pt";
select -ne :renderGlobalsList1;
	setAttr -k on ".cch";
	setAttr -k on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
select -ne :defaultShaderList1;
	setAttr -k on ".cch";
	setAttr -k on ".ihi";
	setAttr -cb on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 11 ".s";
select -ne :postProcessList1;
	setAttr -k on ".cch";
	setAttr -k on ".ihi";
	setAttr -cb on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 2 ".p";
select -ne :defaultRenderUtilityList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 10 ".u";
select -ne :defaultRenderingList1;
	setAttr -k on ".ihi";
	setAttr -s 2 ".r";
select -ne :lightList1;
	setAttr -s 2 ".l";
select -ne :defaultTextureList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 5 ".tx";
select -ne :initialShadingGroup;
	setAttr -av -k on ".cch";
	setAttr -k on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k on ".mwc";
	setAttr -k on ".an";
	setAttr -k on ".il";
	setAttr -k on ".vo";
	setAttr -k on ".eo";
	setAttr -k on ".fo";
	setAttr -k on ".epo";
	setAttr -k on ".ro" yes;
select -ne :initialParticleSE;
	setAttr -av -k on ".cch";
	setAttr -k on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k on ".mwc";
	setAttr -k on ".an";
	setAttr -k on ".il";
	setAttr -k on ".vo";
	setAttr -k on ".eo";
	setAttr -k on ".fo";
	setAttr -k on ".epo";
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
	setAttr -av -k on ".esr";
	setAttr -k on ".ors";
	setAttr -cb on ".sdf";
	setAttr -av ".outf";
	setAttr -k on ".gama";
	setAttr -k on ".an";
	setAttr -k on ".ar";
	setAttr -k on ".fs";
	setAttr -k on ".ef";
	setAttr -av -k on ".bfs";
	setAttr -k on ".me";
	setAttr -k on ".se";
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
	setAttr -k on ".mb";
	setAttr -av -k on ".mbf";
	setAttr -k on ".afp";
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
select -ne :defaultLightSet;
	setAttr -k on ".cch";
	setAttr -k on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -k on ".bnm";
	setAttr -s 2 ".dsm";
	setAttr -k on ".mwc";
	setAttr -k on ".an";
	setAttr -k on ".il";
	setAttr -k on ".vo";
	setAttr -k on ".eo";
	setAttr -k on ".fo";
	setAttr -k on ".epo";
	setAttr -k on ".ro";
select -ne :defaultColorMgtGlobals;
	setAttr ".cfe" yes;
	setAttr ".cfp" -type "string" "<MAYA_RESOURCES>/OCIO-configs/Maya2022-default/config.ocio";
	setAttr ".wsn" -type "string" "ACEScg";
select -ne :hardwareRenderGlobals;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k off -cb on ".ctrs" 256;
	setAttr -av -k off -cb on ".btrs" 512;
	setAttr -k off -cb on ".fbfm";
	setAttr -k off -cb on ".ehql";
	setAttr -k off -cb on ".eams";
	setAttr -k off -cb on ".eeaa";
	setAttr -k off -cb on ".engm";
	setAttr -k off -cb on ".mes";
	setAttr -k off -cb on ".emb";
	setAttr -av -k off -cb on ".mbbf";
	setAttr -k off -cb on ".mbs";
	setAttr -k off -cb on ".trm";
	setAttr -k off -cb on ".tshc";
	setAttr -k off -cb on ".enpt";
	setAttr -k off -cb on ".clmt";
	setAttr -k off -cb on ".tcov";
	setAttr -k off -cb on ".lith";
	setAttr -k off -cb on ".sobc";
	setAttr -k off -cb on ".cuth";
	setAttr -k off -cb on ".hgcd";
	setAttr -k off -cb on ".hgci";
	setAttr -k off -cb on ".mgcs";
	setAttr -k off -cb on ".twa";
	setAttr -k off -cb on ".twz";
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
	setAttr -k on ".cch";
	setAttr -k on ".ihi";
	setAttr -k on ".nds";
	setAttr -k on ".bnm";
	setAttr -av ".gsn";
	setAttr -k on ".gsv";
	setAttr -s 2 ".sol";
connectAttr "rhino_rig_029RN.phl[1]" "rhino_rig_029RN.phl[2]";
connectAttr "cn_neck_jnt_001_cnt_WorldSpace.o" "rhino_rig_029RN.phl[3]";
connectAttr "cn_neck_jnt_001_cnt_rotateX.o" "rhino_rig_029RN.phl[4]";
connectAttr "cn_neck_jnt_001_cnt_rotateY.o" "rhino_rig_029RN.phl[5]";
connectAttr "cn_neck_jnt_001_cnt_rotateZ.o" "rhino_rig_029RN.phl[6]";
connectAttr "cn_chest_jnt_001_cnt_translateX.o" "rhino_rig_029RN.phl[7]";
connectAttr "cn_chest_jnt_001_cnt_translateY.o" "rhino_rig_029RN.phl[8]";
connectAttr "cn_chest_jnt_001_cnt_translateZ.o" "rhino_rig_029RN.phl[9]";
connectAttr "cn_chest_jnt_001_cnt_rotateX.o" "rhino_rig_029RN.phl[10]";
connectAttr "cn_chest_jnt_001_cnt_rotateY.o" "rhino_rig_029RN.phl[11]";
connectAttr "cn_chest_jnt_001_cnt_rotateZ.o" "rhino_rig_029RN.phl[12]";
connectAttr "cn_pelvis_jnt_002_cnt_translateX.o" "rhino_rig_029RN.phl[13]";
connectAttr "cn_pelvis_jnt_002_cnt_translateY.o" "rhino_rig_029RN.phl[14]";
connectAttr "cn_pelvis_jnt_002_cnt_translateZ.o" "rhino_rig_029RN.phl[15]";
connectAttr "cn_pelvis_jnt_002_cnt_rotateX.o" "rhino_rig_029RN.phl[16]";
connectAttr "cn_pelvis_jnt_002_cnt_rotateY.o" "rhino_rig_029RN.phl[17]";
connectAttr "cn_pelvis_jnt_002_cnt_rotateZ.o" "rhino_rig_029RN.phl[18]";
connectAttr "cn_jaw_jnt_001_cnt_translateX.o" "rhino_rig_029RN.phl[19]";
connectAttr "cn_jaw_jnt_001_cnt_translateY.o" "rhino_rig_029RN.phl[20]";
connectAttr "cn_jaw_jnt_001_cnt_translateZ.o" "rhino_rig_029RN.phl[21]";
connectAttr "cn_jaw_jnt_001_cnt_rotateX.o" "rhino_rig_029RN.phl[22]";
connectAttr "cn_jaw_jnt_001_cnt_rotateY.o" "rhino_rig_029RN.phl[23]";
connectAttr "cn_jaw_jnt_001_cnt_rotateZ.o" "rhino_rig_029RN.phl[24]";
connectAttr "lf_ft_scapular_001_cnt_translateX.o" "rhino_rig_029RN.phl[25]";
connectAttr "lf_ft_scapular_001_cnt_translateY.o" "rhino_rig_029RN.phl[26]";
connectAttr "lf_ft_scapular_001_cnt_translateZ.o" "rhino_rig_029RN.phl[27]";
connectAttr "lf_ft_scapular_001_cnt_rotateX.o" "rhino_rig_029RN.phl[28]";
connectAttr "lf_ft_scapular_001_cnt_rotateY.o" "rhino_rig_029RN.phl[29]";
connectAttr "lf_ft_scapular_001_cnt_rotateZ.o" "rhino_rig_029RN.phl[30]";
connectAttr "lf_ft_scapular_002_cnt_translateX.o" "rhino_rig_029RN.phl[31]";
connectAttr "lf_ft_scapular_002_cnt_translateY.o" "rhino_rig_029RN.phl[32]";
connectAttr "lf_ft_scapular_002_cnt_translateZ.o" "rhino_rig_029RN.phl[33]";
connectAttr "lf_ft_scapular_002_cnt_rotateX.o" "rhino_rig_029RN.phl[34]";
connectAttr "lf_ft_scapular_002_cnt_rotateY.o" "rhino_rig_029RN.phl[35]";
connectAttr "lf_ft_scapular_002_cnt_rotateZ.o" "rhino_rig_029RN.phl[36]";
connectAttr "rhino_rig_029RN.phl[37]" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[1].dn"
		;
connectAttr "rhino_rig_029RN.phl[38]" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[0].dn"
		;
connectAttr "lf_ft_foot_001_cnt_translateX.o" "rhino_rig_029RN.phl[39]";
connectAttr "lf_ft_foot_001_cnt_translateY.o" "rhino_rig_029RN.phl[40]";
connectAttr "lf_ft_foot_001_cnt_translateZ.o" "rhino_rig_029RN.phl[41]";
connectAttr "lf_ft_foot_001_cnt_rotateX.o" "rhino_rig_029RN.phl[42]";
connectAttr "lf_ft_foot_001_cnt_rotateY.o" "rhino_rig_029RN.phl[43]";
connectAttr "lf_ft_foot_001_cnt_rotateZ.o" "rhino_rig_029RN.phl[44]";
connectAttr "rt_ft_scapular_001_cnt_translateX.o" "rhino_rig_029RN.phl[45]";
connectAttr "rt_ft_scapular_001_cnt_translateY.o" "rhino_rig_029RN.phl[46]";
connectAttr "rt_ft_scapular_001_cnt_translateZ.o" "rhino_rig_029RN.phl[47]";
connectAttr "rt_ft_scapular_001_cnt_rotateX.o" "rhino_rig_029RN.phl[48]";
connectAttr "rt_ft_scapular_001_cnt_rotateY.o" "rhino_rig_029RN.phl[49]";
connectAttr "rt_ft_scapular_001_cnt_rotateZ.o" "rhino_rig_029RN.phl[50]";
connectAttr "rt_ft_scapular_002_cnt_translateX.o" "rhino_rig_029RN.phl[51]";
connectAttr "rt_ft_scapular_002_cnt_translateY.o" "rhino_rig_029RN.phl[52]";
connectAttr "rt_ft_scapular_002_cnt_translateZ.o" "rhino_rig_029RN.phl[53]";
connectAttr "rt_ft_scapular_002_cnt_rotateX.o" "rhino_rig_029RN.phl[54]";
connectAttr "rt_ft_scapular_002_cnt_rotateY.o" "rhino_rig_029RN.phl[55]";
connectAttr "rt_ft_scapular_002_cnt_rotateZ.o" "rhino_rig_029RN.phl[56]";
connectAttr "rt_ft_foot_001_cnt_translateX.o" "rhino_rig_029RN.phl[57]";
connectAttr "rt_ft_foot_001_cnt_translateY.o" "rhino_rig_029RN.phl[58]";
connectAttr "rt_ft_foot_001_cnt_translateZ.o" "rhino_rig_029RN.phl[59]";
connectAttr "rt_ft_foot_001_cnt_rotateX.o" "rhino_rig_029RN.phl[60]";
connectAttr "rt_ft_foot_001_cnt_rotateY.o" "rhino_rig_029RN.phl[61]";
connectAttr "rt_ft_foot_001_cnt_rotateZ.o" "rhino_rig_029RN.phl[62]";
connectAttr "lf_bk_foot_001_cnt_translateX.o" "rhino_rig_029RN.phl[63]";
connectAttr "lf_bk_foot_001_cnt_translateY.o" "rhino_rig_029RN.phl[64]";
connectAttr "lf_bk_foot_001_cnt_translateZ.o" "rhino_rig_029RN.phl[65]";
connectAttr "lf_bk_foot_001_cnt_rotateX.o" "rhino_rig_029RN.phl[66]";
connectAttr "lf_bk_foot_001_cnt_rotateY.o" "rhino_rig_029RN.phl[67]";
connectAttr "lf_bk_foot_001_cnt_rotateZ.o" "rhino_rig_029RN.phl[68]";
connectAttr "lf_bk_hip_ankle_cnt_rotateX.o" "rhino_rig_029RN.phl[69]";
connectAttr "lf_bk_hip_ankle_cnt_rotateY.o" "rhino_rig_029RN.phl[70]";
connectAttr "lf_bk_hip_ankle_cnt_rotateZ.o" "rhino_rig_029RN.phl[71]";
connectAttr "rt_bk_foot_001_cnt_translateX.o" "rhino_rig_029RN.phl[72]";
connectAttr "rt_bk_foot_001_cnt_translateY.o" "rhino_rig_029RN.phl[73]";
connectAttr "rt_bk_foot_001_cnt_translateZ.o" "rhino_rig_029RN.phl[74]";
connectAttr "rt_bk_foot_001_cnt_rotateX.o" "rhino_rig_029RN.phl[75]";
connectAttr "rt_bk_foot_001_cnt_rotateY.o" "rhino_rig_029RN.phl[76]";
connectAttr "rt_bk_foot_001_cnt_rotateZ.o" "rhino_rig_029RN.phl[77]";
connectAttr "rt_bk_hip_ankle_cnt_rotateX.o" "rhino_rig_029RN.phl[78]";
connectAttr "rt_bk_hip_ankle_cnt_rotateY.o" "rhino_rig_029RN.phl[79]";
connectAttr "rt_bk_hip_ankle_cnt_rotateZ.o" "rhino_rig_029RN.phl[80]";
connectAttr "rt_bk_hip_ankle_cnt_translateX.o" "rhino_rig_029RN.phl[81]";
connectAttr "rt_bk_hip_ankle_cnt_translateY.o" "rhino_rig_029RN.phl[82]";
connectAttr "rt_bk_hip_ankle_cnt_translateZ.o" "rhino_rig_029RN.phl[83]";
connectAttr "cn_tail_jnt_001_cnt_worldspace.o" "rhino_rig_029RN.phl[84]";
connectAttr "cn_tail_jnt_001_cnt_translateX.o" "rhino_rig_029RN.phl[85]";
connectAttr "cn_tail_jnt_001_cnt_translateY.o" "rhino_rig_029RN.phl[86]";
connectAttr "cn_tail_jnt_001_cnt_translateZ.o" "rhino_rig_029RN.phl[87]";
connectAttr "cn_tail_jnt_001_cnt_rotateX.o" "rhino_rig_029RN.phl[88]";
connectAttr "cn_tail_jnt_001_cnt_rotateY.o" "rhino_rig_029RN.phl[89]";
connectAttr "cn_tail_jnt_001_cnt_rotateZ.o" "rhino_rig_029RN.phl[90]";
connectAttr "cn_tail_jnt_002_cnt_translateX.o" "rhino_rig_029RN.phl[91]";
connectAttr "cn_tail_jnt_002_cnt_translateY.o" "rhino_rig_029RN.phl[92]";
connectAttr "cn_tail_jnt_002_cnt_translateZ.o" "rhino_rig_029RN.phl[93]";
connectAttr "cn_tail_jnt_002_cnt_rotateX.o" "rhino_rig_029RN.phl[94]";
connectAttr "cn_tail_jnt_002_cnt_rotateY.o" "rhino_rig_029RN.phl[95]";
connectAttr "cn_tail_jnt_002_cnt_rotateZ.o" "rhino_rig_029RN.phl[96]";
connectAttr "cn_tail_jnt_003_cnt_translateX.o" "rhino_rig_029RN.phl[97]";
connectAttr "cn_tail_jnt_003_cnt_translateY.o" "rhino_rig_029RN.phl[98]";
connectAttr "cn_tail_jnt_003_cnt_translateZ.o" "rhino_rig_029RN.phl[99]";
connectAttr "cn_tail_jnt_003_cnt_rotateX.o" "rhino_rig_029RN.phl[100]";
connectAttr "cn_tail_jnt_003_cnt_rotateY.o" "rhino_rig_029RN.phl[101]";
connectAttr "cn_tail_jnt_003_cnt_rotateZ.o" "rhino_rig_029RN.phl[102]";
connectAttr "cn_tail_jnt_004_cnt_translateX.o" "rhino_rig_029RN.phl[103]";
connectAttr "cn_tail_jnt_004_cnt_translateY.o" "rhino_rig_029RN.phl[104]";
connectAttr "cn_tail_jnt_004_cnt_translateZ.o" "rhino_rig_029RN.phl[105]";
connectAttr "cn_tail_jnt_004_cnt_rotateX.o" "rhino_rig_029RN.phl[106]";
connectAttr "cn_tail_jnt_004_cnt_rotateY.o" "rhino_rig_029RN.phl[107]";
connectAttr "cn_tail_jnt_004_cnt_rotateZ.o" "rhino_rig_029RN.phl[108]";
connectAttr "cn_tail_jnt_005_cnt_translateX.o" "rhino_rig_029RN.phl[109]";
connectAttr "cn_tail_jnt_005_cnt_translateY.o" "rhino_rig_029RN.phl[110]";
connectAttr "cn_tail_jnt_005_cnt_translateZ.o" "rhino_rig_029RN.phl[111]";
connectAttr "cn_tail_jnt_005_cnt_rotateX.o" "rhino_rig_029RN.phl[112]";
connectAttr "cn_tail_jnt_005_cnt_rotateY.o" "rhino_rig_029RN.phl[113]";
connectAttr "cn_tail_jnt_005_cnt_rotateZ.o" "rhino_rig_029RN.phl[114]";
connectAttr "rhino_rig_029RN.phl[115]" "rhino_rig_029RN.phl[116]";
relationship "link" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
connectAttr "layerManager.dli[0]" "defaultLayer.id";
connectAttr "renderLayerManager.rlmi[0]" "defaultRenderLayer.rlid";
connectAttr "sharedReferenceNode.sr" "rhino_rig_029RN.sr";
connectAttr "defaultRenderLayer.msg" ":defaultRenderingList1.r" -na;
// End of anim_001.ma
