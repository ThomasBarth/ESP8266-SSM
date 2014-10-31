<?php
	//try to parse parameter "d" as an integer, prevent SQl injections
	$duration=intval($_GET['d']);

	//if we get a both parameters 
	if($duration && $_GET['o'])
	{
		//get the weather for the given location, get an APPID at openweater.org																					
		$json = file_get_contents('http://api.openweathermap.org/data/2.5/weather?q=' . $_GET['o'] .'&mode=json&units=metric&APPID=<YOUR_APPID>');
		
		//parse the JSON from the request
		$weather = json_decode($json);	

		//check if location had been found
		if($weather->cod!=404)
		{
			//create new mySQL connection
			$con=new mysqli("<YOUR_SQLSERVER>","<YOUR_SQLUSER>","<YOUR_SQLPASSWD>","<YOUR_SQLDB>");

			// Check if connection was established
			if (mysqli_connect_errno()) 
			  echo "Failed to connect to MySQL: " . mysqli_connect_error();	

			//execute SQL query
			mysqli_query($con,"INSERT INTO heating (moment, duration, outside_temp)VALUES ('".date("Y-m-d H:i:s")."', '". $duration ."','" . $weather->main->temp . "')");

			//close database connection
			mysqli_close($con);
			
			echo "success";
		}
		else
			echo "location was not found";
	}
	else
		echo "missing or wrong parameters";
?>