<?php
$conn = mysqli_connect("localhost", "root", "", "my_database");


$json = file_get_contents('php://input');
$data = json_decode($json);
// var_dump($data);

// $tableName = data->frame_id;

// $val = mysql_query("SELECT 1 FROM $tableName LIMIT 1");

// if($val !== FALSE)
// {
//    //DO SOMETHING! IT EXISTS!
// }
// else
// {
//     //I can't find it...
// }

$frame_id = $data->cms_data[0]->frame_id;
$bid = $data->cms_data[0]->bid;
$vcell = $data->cms_data[0]->vcell;
$temp = $data->cms_data[0]->temp;
$pack = $data->cms_data[0]->pack;
$wake_status = $data->cms_data[0]->wake_status;


var_dump($frame_id);
var_dump($bid);
var_dump($vcell[0]);
var_dump($temp[0]);
var_dump($pack[0]);
var_dump($wake_status);

// $sql = "INSERT INTO temperature (temperature_1) VALUES ($value)";
// $result = mysqli_query($conn, $sql);

// if (!$result) {
// 	echo mysql_error();
// }

// echo "<h2>PHP is Fun!</h2>";
// echo "Hello world!<br>";
// echo "I'm about to learn PHP!<br>";
// echo "This ", "string ", "was ", "made ", "with multiple parameters.";
?>