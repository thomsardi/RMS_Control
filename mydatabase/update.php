<?php

$json = file_get_contents('php://input');
$data = json_decode($json);


$databaseName = "charging_station";
$frameName = $data->frame_name;
$bid = $data->bid;
$vcell = $data->vcell;
$temp = $data->temp;
$pack = $data->pack;
$wake_status = $data->wake_status;
$tableName = $frameName;
$column_list = "";
$value_list = "";

$columns = "bid";
$values = $bid;

$columns .= ",";
$values .= ",";

for ($x = 1; $x <= 45; $x++) {
  $column_list .= "v";
  $column_list .= $x;
  if ($x != 45) {
  	$column_list .= ",";
  } 
}

for ($x = 0; $x < 45; $x++) {
  $value_list .= $vcell[$x];
  if ($x != 44) {
  	$value_list .= ",";
  } 
}

$columns .= $column_list;
$columns .= ",";

$values .= $value_list;
$values .= ",";

$column_list = "";
$value_list = "";

for ($x = 1; $x <= 9; $x++) {
  $column_list .= "t";
  $column_list .= $x;
  if ($x != 9) {
    $column_list .= ",";
  } 
}

for ($x = 0; $x < 9; $x++) {
  $value_list .= $temp[$x];
  if ($x != 8) {
    $value_list .= ",";
  } 
}

$columns .= $column_list;
$columns .= ",";

$values .= $value_list;
$values .= ",";

$column_list = "";
$value_list = "";


for ($x = 1; $x <= 3; $x++) {
  $column_list .= "vp";
  $column_list .= $x;
  if ($x != 3) {
    $column_list .= ",";
  } 
}

for ($x = 0; $x < 3; $x++) {
  $value_list .= $pack[$x];
  if ($x != 2) {
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
$values .= $wake_status;

$conn = mysqli_connect("localhost", "root", "", $databaseName);
$sql = "INSERT INTO $tableName ($columns) VALUES ($values)";
echo $sql;
$result = mysqli_query($conn, $sql);

if (!$result) {
	echo mysql_error();
}


?>