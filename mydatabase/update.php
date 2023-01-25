<?php

require 'function.php';

$json = file_get_contents('php://input');
$data = json_decode($json);

// $databaseName = "charging_station";
$msgCount = $data->msg_count;
$frameName = $data->frame_name;
$cmsCode = $data->cms_code;
$baseCode = $data->base_code;
$mcuCode = $data->mcu_code;
$siteLocation = $data->site_location;
$bid = $data->bid;
$vcell = $data->vcell;
$temp = $data->temp;
$pack = $data->pack;
$wake_status = $data->wake_status;
$door_status = $data->door_status;
$tableName = $frameName;

$columns = "msg_count";
$values = $msgCount;

$columns .= ",";
$values .= ",";

$columns .= "cms_code";
$values .= "'" . $cmsCode . "'";

$columns .= ",";
$values .= ",";

$columns .= "base_code";
$values .= "'" . $baseCode . "'";

$columns .= ",";
$values .= ",";

$columns .= "mcu_code";
$values .= "'" . $mcuCode . "'";

$columns .= ",";
$values .= ",";

$columns .= "site_location";
$values .= "'" . $siteLocation . "'";

$columns .= ",";
$values .= ",";

$columns .= "bid";
$values .= $bid;

$columns .= ",";
$values .= ",";

$column_list = "";
$value_list = "";
$element_count = count($vcell);
for ($x = 0; $x < $element_count; $x++) {
  $column_list .= "v";
  $column_list .= $x+1;
  $value_list .= $vcell[$x];
  if ($x != ($element_count - 1)) 
  {
    $column_list .= ",";
    $value_list .= ",";
  } 
}

$columns .= $column_list;
$columns .= ",";

$values .= $value_list;
$values .= ",";

$column_list = "";
$value_list = "";
$element_count = count($temp);
for ($x = 0; $x < $element_count; $x++) {
  $column_list .= "t";
  $column_list .= $x+1;
  $value_list .= $temp[$x];
  if ($x != ($element_count - 1)) {
    $column_list .= ",";
    $value_list .= ",";
  } 
}

$columns .= $column_list;
$columns .= ",";

$values .= $value_list;
$values .= ",";

$column_list = "";
$value_list = "";
$element_count = count($pack);
for ($x = 0; $x < $element_count; $x++) {
  $column_list .= "vp";
  $column_list .= $x+1;
  $value_list .= $pack[$x];
  if ($x != ($element_count-1)) {
    $column_list .= ",";
    $value_list .= ",";
  } 
}

$columns .= $column_list;
$columns .= ",";
$values .= $value_list;
$values .= ",";

$column_list = "";
$value_list = "";

$columns .= "wake_status";
$columns .= ",";
$values .= $wake_status;
$values .= ",";

$columns .= "door_status";
$values .= $door_status;

// $conn = mysqli_connect("localhost", "root", "", $databaseName);
$sql = "INSERT INTO `$tableName` ($columns) VALUES ($values)";
echo $sql;
// $result = mysqli_query($conn, $sql);
$result = queryDatabase($sql);

if (!$result) {
	echo mysql_error();
}

?>