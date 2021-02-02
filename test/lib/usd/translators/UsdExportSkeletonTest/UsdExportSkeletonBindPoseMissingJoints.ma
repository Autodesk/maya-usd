//Maya ASCII 2020 scene
//Name: UsdExportSkeletonBindPoseMissingJoints.ma
//Last modified: Wed, Jan 27, 2021 02:16:36 PM
//Codeset: UTF-8
requires maya "2020";
requires "stereoCamera" "10.0";
requires "stereoCamera" "10.0";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2020";
fileInfo "version" "2020";
fileInfo "cutIdentifier" "202011110415-b1e20b88e2";
fileInfo "osv" "Linux 3.10.0-1062.18.1.el7.x86_64 #1 SMP Tue Mar 17 23:49:17 UTC 2020 x86_64";
fileInfo "UUID" "CA508C80-0000-5159-6011-E644000002A4";
createNode transform -n "joint_grp";
	rename -uid "716A8C80-0000-676C-6011-C976000006DF";
createNode joint -n "joint1" -p "joint_grp";
	rename -uid "716A8C80-0000-676C-6011-C839000006BD";
	setAttr ".t" -type "double3" -0.13387224879650539 0.21395404220941927 0 ;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 0 0 2.5352931362020645 ;
	setAttr ".radi" 0.66868990097983616;
createNode joint -n "joint2" -p "joint1";
	rename -uid "716A8C80-0000-676C-6011-C83A000006BE";
	setAttr ".t" -type "double3" 4.2613380856101655 8.8262730457699945e-15 0 ;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 0 0 -50.479340742609303 ;
	setAttr ".radi" 0.60456769613120953;
createNode joint -n "joint3" -p "joint2";
	rename -uid "716A8C80-0000-676C-6011-C83A000006BF";
	setAttr ".t" -type "double3" 3.0216421252033836 6.6613381477509392e-16 0 ;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 0 0 47.94404760640726 ;
	setAttr ".radi" 0.60456769613120953;
createNode joint -n "joint4" -p "joint2";
	rename -uid "716A8C80-0000-676C-6011-C857000006C2";
	setAttr ".t" -type "double3" 3.0216421252033836 1.7763568394002505e-15 0 ;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 0 0 47.944047606407246 ;
	setAttr ".radi" 0.60456769613120953;
createNode dagPose -n "dagPose1";
	rename -uid "716A8C80-0000-676C-6011-C87A000006C4";
	setAttr -s 3 ".wm";
	setAttr ".wm[0]" -type "matrix" 0.99902116331496138 0.044234774203348912 0 0 -0.044234774203348912 0.99902116331496138 0 0
		 0 0 1 0 -0.13387224879650539 0.21395404220941927 0 1;
	setAttr ".wm[1]" -type "matrix" 0.66985600785788646 -0.74249102939813039 0 0 0.74249102939813039 0.66985600785788646 0 0
		 0 0 1 0 4.1232946827681127 0.40245337023052485 0 1;
	setAttr ".wm[2]" -type "matrix" 1.0000000000000002 2.7755575615628914e-16 0 0 -2.7755575615628914e-16 1.0000000000000002 0 0
		 0 0 1 0 6.1473598139320718 -1.8410888017844895 0 1;
	setAttr -s 3 ".xm";
	setAttr ".xm[0]" -type "matrix" "xform" 1 1 1 0 0 0 0 -0.13387224879650539
		 0.21395404220941927 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0 0 0.022122801416622397 0.9997552608801219 1
		 1 1 yes;
	setAttr ".xm[1]" -type "matrix" "xform" 1 1 1 0 0 0 0 4.2613380856101655
		 8.8262730457699945e-15 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0 0 -0.42640567234133786 0.90453203514034353 1
		 1 1 yes;
	setAttr ".xm[2]" -type "matrix" "xform" 1 1 1 0 0 0 0 3.0216421252033836
		 6.6613381477509392e-16 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0 0 0.40629053160399503 0.91374394877829046 1
		 1 1 yes;
	setAttr -s 3 ".m";
	setAttr -s 3 ".p";
connectAttr "joint1.s" "joint2.is";
connectAttr "joint2.s" "joint3.is";
connectAttr "joint2.s" "joint4.is";
connectAttr "joint1.msg" "dagPose1.m[0]";
connectAttr "joint2.msg" "dagPose1.m[1]";
connectAttr "joint3.msg" "dagPose1.m[2]";
connectAttr "dagPose1.w" "dagPose1.p[0]";
connectAttr "dagPose1.m[0]" "dagPose1.p[1]";
connectAttr "dagPose1.m[1]" "dagPose1.p[2]";
// End of UsdExportSkeletonBindPoseMissingJoints.ma
