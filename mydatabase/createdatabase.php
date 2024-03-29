<?php 

require 'function.php';

// $conn = mysqli_connect("localhost", "root", "");
$json = file_get_contents('php://input');
$data = json_decode($json);
// $vcell = $data->cms_data[0]->vcell;
$databaseName = "charging_station";
$frameName = $data->frame_name;

// if (!is_string($frameName)) {
//   $temp = strval($frameName);
//   $frameName = "'" . $temp . "'";
// }

/*
$sql = "CREATE DATABASE IF NOT EXISTS $databaseName";

$result = mysqli_query($conn, $sql);

if (!$result) {
	echo mysql_error();
}

$conn = mysqli_connect("localhost", "root", "", $databaseName);

if ($conn -> connect_errno) {
  echo "Failed to connect to MySQL: " . $conn -> connect_error;
  exit();
}*/

$result = createDatabase();

if($result < 0)
{
  echo_ln("Could not connect to $databaseName database");
  echo_ln("Exit");
  exit();
}
else {
  echo_ln("$databaseName database created successfully");
}

$sql_create_table = "CREATE TABLE `$frameName` (
 `id` int(11) NOT NULL AUTO_INCREMENT,
 `timestamp` timestamp NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
 `msg_count` INT UNSIGNED NOT NULL,
 `frame_name` varchar(50) NOT NULL,
 `cms_code` VARCHAR(50) NOT NULL,
 `base_code` VARCHAR(50) NOT NULL,
 `mcu_code` VARCHAR(50) NOT NULL,
 `site_location` VARCHAR(50) NOT NULL,
 `bid` int(11) NOT NULL,
 `v1` int(11) NOT NULL,
 `v2` int(11) NOT NULL,
 `v3` int(11) NOT NULL,
 `v4` int(11) NOT NULL,
 `v5` int(11) NOT NULL,
 `v6` int(11) NOT NULL,
 `v7` int(11) NOT NULL,
 `v8` int(11) NOT NULL,
 `v9` int(11) NOT NULL,
 `v10` int(11) NOT NULL,
 `v11` int(11) NOT NULL,
 `v12` int(11) NOT NULL,
 `v13` int(11) NOT NULL,
 `v14` int(11) NOT NULL,
 `v15` int(11) NOT NULL,
 `v16` int(11) NOT NULL,
 `v17` int(11) NOT NULL,
 `v18` int(11) NOT NULL,
 `v19` int(11) NOT NULL,
 `v20` int(11) NOT NULL,
 `v21` int(11) NOT NULL,
 `v22` int(11) NOT NULL,
 `v23` int(11) NOT NULL,
 `v24` int(11) NOT NULL,
 `v25` int(11) NOT NULL,
 `v26` int(11) NOT NULL,
 `v27` int(11) NOT NULL,
 `v28` int(11) NOT NULL,
 `v29` int(11) NOT NULL,
 `v30` int(11) NOT NULL,
 `v31` int(11) NOT NULL,
 `v32` int(11) NOT NULL,
 `v33` int(11) NOT NULL,
 `v34` int(11) NOT NULL,
 `v35` int(11) NOT NULL,
 `v36` int(11) NOT NULL,
 `v37` int(11) NOT NULL,
 `v38` int(11) NOT NULL,
 `v39` int(11) NOT NULL,
 `v40` int(11) NOT NULL,
 `v41` int(11) NOT NULL,
 `v42` int(11) NOT NULL,
 `v43` int(11) NOT NULL,
 `v44` int(11) NOT NULL,
 `v45` int(11) NOT NULL,
 `t1` int(11) NOT NULL,
 `t2` int(11) NOT NULL,
 `t3` int(11) NOT NULL,
 `t4` int(11) NOT NULL,
 `t5` int(11) NOT NULL,
 `t6` int(11) NOT NULL,
 `t7` int(11) NOT NULL,
 `t8` int(11) NOT NULL,
 `t9` int(11) NOT NULL,
 `vp1` int(11) NOT NULL,
 `vp2` int(11) NOT NULL,
 `vp3` int(11) NOT NULL,
 `wake_status` int(11) NOT NULL,
 `door_status` int(11) NOT NULL,
 PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci";

$query = "SHOW TABLES LIKE '$frameName'";
$result = queryDatabase($query);
$tableExist = $result->num_rows;
// echo $query;
// $result = mysqli_query($conn, $query);
// $tableExist = $result->num_rows;

if($tableExist <= 0) {
  // $result = mysqli_query($conn, $sql_create_table);
  // echo "Table Not Exist";
  $result = queryDatabase($sql_create_table);
  if ($result) {
    echo_ln("$frameName table created successfully");
  }
}
else {
  echo_ln("$frameName Table Exist");
}

?>