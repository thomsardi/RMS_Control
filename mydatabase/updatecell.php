<?php

$json = file_get_contents('php://input');
$data = json_decode($json);

$frameName = $data->frame_name;
$bid = $data->BID;
$vcell = $data->VCELL;
$tableName = "vcell";
$column_list = "";
$value_list = "";

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


$columns = $column_list;
$values = $value_list;

$conn = mysqli_connect("localhost", "root", "", $frameName);
$sql = "INSERT INTO $tableName ($columns) VALUES ($values)";
echo $sql;
$result = mysqli_query($conn, $sql);

if (!$result) {
	echo mysql_error();
}

$tableName = "bid";
$columns = "bid";
$values = $bid;

$sql = "INSERT INTO $tableName ($columns) VALUES ($values)";
echo $sql;
$result = mysqli_query($conn, $sql);

if (!$result) {
  echo mysql_error();
}

?>