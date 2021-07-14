//Maya ASCII 2018 scene
//Name: UsdExportRootTest.ma
//Last modified: Fri, Apr 20, 2018 02:26:40 PM
//Codeset: UTF-8
requires maya "2018";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2018";
fileInfo "version" "2018";
fileInfo "cutIdentifier" "201708311015-002f4fe637";
fileInfo "osv" "Linux 3.10.0-514.6.1.el7.x86_64 #1 SMP Wed Jan 18 13:06:36 UTC 2017 x86_64";
createNode transform -n "Top";
	rename -uid "D0BBC940-0000-4EBB-5AD5-491C00000255";
createNode transform -n "Mid" -p "Top";
	rename -uid "D0BBC940-0000-4EBB-5AD5-491B00000254";
createNode transform -n "Cube" -p "Mid";
	rename -uid "D0BBC940-0000-4EBB-5AD5-491A00000253";
createNode mesh -n "CubeShape" -p "Cube";
	rename -uid "D0BBC940-0000-4EBB-5AD5-491A00000252";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
createNode transform -n "OtherLowest" -p "Mid";
	rename -uid "BDCE9940-0000-7302-5ADA-417700000269";
createNode transform -n "OtherMid" -p "Top";
	rename -uid "BDCE9940-0000-7302-5ADA-416500000268";
createNode transform -n "OtherTop";
	rename -uid "BDCE9940-0000-7302-5ADA-3FAD00000267";
createNode polyCube -n "polyCube1";
	rename -uid "D0BBC940-0000-4EBB-5AD5-491A00000251";
	setAttr ".cuv" 4;
connectAttr "polyCube1.out" "CubeShape.i";
connectAttr "CubeShape.iog" ":initialShadingGroup.dsm" -na;
// End of UsdExportRootTest.ma
