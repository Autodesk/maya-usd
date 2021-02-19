//Maya ASCII 2020 scene
//Name: UsdExportSkeletonNoDagPose.ma
//Last modified: Wed, Jan 27, 2021 05:20:36 PM
//Codeset: UTF-8
requires maya "2020";
requires "stereoCamera" "10.0";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2020";
fileInfo "version" "2020";
fileInfo "cutIdentifier" "202011110415-b1e20b88e2";
fileInfo "osv" "Linux 3.10.0-1062.18.1.el7.x86_64 #1 SMP Tue Mar 17 23:49:17 UTC 2020 x86_64";
fileInfo "UUID" "075DCC80-0000-2CD0-6012-11640000043B";
createNode transform -n "skel_root";
	rename -uid "075DCC80-0000-2CD0-6012-11480000043A";
createNode joint -n "joint1" -p "skel_root";
	rename -uid "075DCC80-0000-2CD0-6012-0D7900000396";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".t" -type "double3" -8 1 7 ;
	setAttr ".r" -type "double3" 0 60 0 ;
	setAttr ".bps" -type "matrix" -1 0 0 0 0 1 0 0
		 0 0 -1 0 0 0 0 1;
createNode joint -n "joint2" -p "joint1";
	rename -uid "075DCC80-0000-2CD0-6012-0D7E00000397";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".t" -type "double3" -3 0 1.23 ;
	setAttr ".r" -type "double3" 30 0 0 ;
	setAttr ".bps" -type "matrix" 0 -1 0 0 -1 0 0 0
		 0 0 -1 0 3 0 0 1;
createNode joint -n "joint3" -p "joint2";
	rename -uid "075DCC80-0000-2CD0-6012-0D94000003A6";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".t" -type "double3" -5 5 13 ;
	setAttr ".r" -type "double3" 45 0 45 ;
	setAttr ".bps" -type "matrix" 0 -1 0 0 0 0 -1 0
		 1 0 0 0 3 0 -2 1;
createNode joint -n "joint4" -p "joint3";
	rename -uid "075DCC80-0000-2CD0-6012-0E6A000003FD";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".t" -type "double3" 0 1.9999999999999998 4.4408920985006262e-16 ;
	setAttr ".r" -type "double3" 90 0 0 ;
	setAttr ".bps" -type "matrix" 0 -1 0 0 1 0 0 0
		 0 0 1 0 3 0 -4 1;
connectAttr "joint1.s" "joint2.is";
connectAttr "joint2.s" "joint3.is";
connectAttr "joint3.s" "joint4.is";
// End of UsdExportSkeletonNoDagPose.ma
