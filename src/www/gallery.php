<!DOCTYPE html>
<html>
<head>
  <title>ESP32-CAM Photo Gallery</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
    html,
    body {
      margin: 0;
      padding: 0;
      font-family: Arial, sans-serif;
    }

    body {
      font-size: 1.2em;
      background: linear-gradient(90deg, #2c3e50, #3498db);
      color: #fff;
    }

    .navbar {
      background: linear-gradient(90deg, #3498db, #e74c3c);
      text-align: center;
      padding: 10px;
    }

    h2 {
      margin: 0;
      color: #fff;
    }

    .flex-container {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      padding: 20px;
    }

    .image-box {
      text-align: center;
      margin: 10px;
      padding: 10px;
      border: 1px solid #ddd;
      border-radius: 5px;
      background-color: #34495e;
      box-shadow: 0 0 10px rgba(52, 73, 94, 0.5);
    }

    .image-box img {
      width: 100%;
      max-width: 350px;
      height: auto;
      border: 1px solid #ddd;
      border-radius: 4px;
      margin-bottom: 8px;
    }

    .delete-button {
      background-color: #e74c3c;
      color: white;
      padding: 8px 12px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
    }
  </style>
</head>
<body>

<div class="navbar">
  <h2>ESP32-CAM Photo Gallery</h2>
</div>

<div class="flex-container">
  <?php
    // Image extensions
    $image_extensions = array("png","jpg","jpeg","gif");

    // Check delete HTTP GET request - remove images
    if(isset($_GET["delete"])){
      $imageFileType = strtolower(pathinfo($_GET["delete"],PATHINFO_EXTENSION));
      if (file_exists($_GET["delete"]) && ($imageFileType == "jpg" ||  $imageFileType == "png" ||  $imageFileType == "jpeg") ) {
        echo "File found and deleted: " .  $_GET["delete"];
        unlink($_GET["delete"]);
        
        header("Location: gallery.php");
      }
      else {
        //redirect to gallery page
        header("Location: gallery.php");
        echo 'File not found - <a href="gallery.php">refresh</a>';
      }
    }
    // Target directory
    $dir = 'uploads/';
    if (is_dir($dir)){
      $count = 1;
      $files = scandir($dir);
      rsort($files);
      foreach ($files as $file) {
        if ($file != '.' && $file != '..') {?>
      <div class="image-box">
        <a href="<?php echo $dir . $file; ?>">
          <img src="<?php echo $dir . $file; ?>" alt="" title=""/>
        </a>
        <p>Caption: <?php echo pathinfo($file, PATHINFO_FILENAME); ?></p>
        <p><?php
          $jsonFile = pathinfo($file, PATHINFO_FILENAME)  . ".json";
          if (file_exists("JSON\\" . $jsonFile)) {
            $json = file_get_contents("JSON\\" . $jsonFile);
            $json = json_decode($json);

            if (json_last_error() == JSON_ERROR_NONE) {
              foreach ($json as $obj) {
                if (isset($obj->label) && isset($obj->value)) {
                echo $obj->label . " ";
                echo $obj->value . " ";
                echo "<br>";
                }
              } 
            } else {
              echo "No predictions";
            }
          }
          else {
            echo "No predictions";
          }

        ?></p>
        <p><button class="delete-button" onclick="deleteImage('<?php echo $dir . $file; ?>')">Delete file</button></p>
      </div>
  <?php
         $count++;
        }
      }
    }
    if($count==1) { echo "<p>No images found</p>"; } 
  ?>
</div>

<script>
  function deleteImage(imagePath) {
    if (confirm("Are you sure you want to delete this image?")) {
      window.location.href = "gallery.php?delete=" + imagePath;
    }
  }
</script>

</body>
</html>
