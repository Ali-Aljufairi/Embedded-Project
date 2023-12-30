<?php
// Rui Santos
// Complete project details at https://RandomNerdTutorials.com/esp32-cam-post-image-photo-server/
// Code Based on this example: w3schools.com/php/php_file_upload.asp

$target_dir = "uploads\\";
$datum = mktime(date('H')+0, date('i'), date('s'), date('m'), date('d'), date('y'));

$imageName = date('Y-m-d_H_i_s-', $datum) . basename($_FILES["imageFile"]["name"]);
$target_file = $target_dir . $imageName;
$uploadOk = 1;
$imageFileType = strtolower(pathinfo($target_file,PATHINFO_EXTENSION));


// invalid file characters check
if (preg_match('/[\'^£$%&*()}{@#~?><>,|=+¬-]/', $imageName))
{
  // remove invalid characters
  $imageName = preg_replace('/[\'^£$%&*()}{@#~?><>,|=+¬-]/', '', $imageName);
}


// Check if image file is a actual image or fake image
if(isset($_POST["submit"])) {
  $check = getimagesize($_FILES["imageFile"]["tmp_name"]);
  if($check !== false) {
    echo "File is an image - " . $check["mime"] . ".";
    $uploadOk = 1;
  }
  else {
    echo "File is not an image.";
    $uploadOk = 0;
  }

}

// Check if file already exists
if (file_exists($target_file)) {
  echo "Sorry, file already exists.";
  $uploadOk = 0;
}

// Check file size
if ($_FILES["imageFile"]["size"] > 500000) {
  echo "Sorry, your file is too large.";
  $uploadOk = 0;
}

// Allow certain file formats
if($imageFileType != "jpg" && $imageFileType != "png" && $imageFileType != "jpeg"
&& $imageFileType != "gif" ) {
  echo "Sorry, only JPG, JPEG, PNG & GIF files are allowed.";
  $uploadOk = 0;
}



// Check if $uploadOk is set to 0 by an error
if ($uploadOk == 0) {
  echo "Sorry, your file was not uploaded.";
// if everything is ok, try to upload file
} else {
  if (move_uploaded_file($_FILES["imageFile"]["tmp_name"], "$target_file")) {
    echo "The file ". basename( $_FILES["imageFile"]["name"]). " has been uploaded.";


  // get boundingBoxes field
  $boundingBoxes = $_POST["boundingBoxes"];
      
      // Get the boundingBoxes field from the POST data
      $boundingBoxes = $_POST["boundingBoxes"];
  
      // Specify the directory path where you want to save the data
      $directoryPath = 'JSON/';
  
      // Ensure the directory exists; create it if not
      if (!file_exists($directoryPath)) {
          mkdir($directoryPath, 0777, true);
      }
  
      // Specify the file path within the directory
      $filePath = $directoryPath . pathinfo($target_file, PATHINFO_FILENAME) . '.json';
  
      // Save the boundingBoxes field to a JSON file
      file_put_contents($filePath, $boundingBoxes);
  
      // Optionally, you can add some feedback or response
      echo 'Bounding boxes data saved to JSON file successfully!';
  





  }
  else {
    echo "Sorry, there was an error uploading your file.";
  }
}






// Check if the request method is POST
// if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    
//     // Read the raw JSON data from the request body
//     $json_data = file_get_contents("php://input");

//     // Decode the JSON data
//     $decoded_data = json_decode($json_data, true);

//     // Check if JSON decoding was successful
//     if ($decoded_data !== null) {
//         // Specify the path where you want to save the JSON file
//         $json_folder = 'JSON/';
//         $json_filename = 'received_data_' . time() . '.json';
//         $json_filepath = $json_folder . $json_filename;

//         // Save the JSON data to the file
//         file_put_contents($json_filepath, $json_data);

//         // Respond with a success message and the path to the saved file
//         $response = array('status' => 'success', 'message' => 'JSON data received and saved successfully', 'file_path' => $json_filepath);
//         echo json_encode($response);
//     } else {
//         // Respond with an error message for invalid JSON
//         $response = array('status' => 'error', 'message' => 'Invalid JSON data');
//         echo json_encode($response);
//     }

// } else {
//     // Respond with an error message for unsupported request method
//     $response = array('status' => 'error', 'message' => 'Unsupported request method');
//     echo json_encode($response);
// }



// redirect to gallery page
// header("Location: gallery.php");
  
?>
