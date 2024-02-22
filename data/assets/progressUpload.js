document.getElementById("uploadForm").addEventListener("submit", function (e) {
  e.preventDefault();
  const formData = new FormData(this);
  const progressBar = document.getElementById("progressBar");
  const progressStatus = document.getElementById("progressStatus");
  const progressWrapper = document.getElementById("progressWrapper");
  const flashRadioButton = document.getElementById("flashradio");
  const filesystemRadioButton = document.getElementById("filesystemradio");
  const progressGauge = progressStatus.querySelector("#barGauge");
  const labelProgress = progressStatus.querySelector("#labelProgress");
  const uploadButton = document.getElementById("uploadButton");
  var fileInput = document.getElementById("fileToUpload");
  var fileName = "";
  var fileExtension = "";

  if (fileInput.files.length) {
    console.log("file detected");
    progressWrapper.classList.add("visible-fadein");
    progressWrapper.classList.remove("hidden-fadeout");
    uploadButton.disabled = true;
    fileName = fileInput.value;
    fileExtension = fileName.split(".").pop();
  }

  const xhr = new XMLHttpRequest();
  // xhr.open("POST", "upload-handler.php", true);
  if (flashRadioButton.checked) {
    console.log("Flash button checked");
    if (fileExtension == "zz") {
      xhr.open("POST", "/update-compressed-firmware", true);
    }
    else {
      xhr.open("POST", "/update-firmware", true);
    }
  }
  if (filesystemRadioButton.checked) {
    console.log("Filesystem button checked");
    if (fileExtension == "zz") {
      xhr.open("POST", "/update-compressed-filesystem", true);
    }
    else {
      xhr.open("POST", "/update-filesystem", true);
    }
    
  }
  
  xhr.upload.onprogress = function (e) {
    if (fileInput.files.length > 0) {
      console.log("File inputted");
      if (e.lengthComputable) {
        const percentComplete = (e.loaded / e.total) * 100;
        progressBar.style.width = percentComplete + "%";
        labelProgress.textContent = percentComplete.toFixed(2) + "%";
        // progressStatus.innerHTML = percentComplete.toFixed(2) + "%";
        progressGauge.value = percentComplete;
        // console.log(percentComplete);
      }
    }
    else {
      console.log("No File inputted");
    }
    
  };

  xhr.onload = function () {
    if (xhr.status === 200) {
      // contentText.textContent = xhr.responseText;
      // Check if the response contains a success message
      console.log("response text : ", xhr.responseText);
      if (xhr.responseText.includes("File has been uploaded successfully.")) {
        // Redirect to a success page
        alert("upload success");
        window.location.href = "/update";
      }
      else {
        alert("upload failed");
      }
      
    } 
    else {
      alert("Response error");
      console.error("Error:", xhr.status);
    }
    uploadButton.disabled = false;
    progressWrapper.classList.remove("visible-fadein");
    progressWrapper.classList.add("hidden-fadeout");
  };

  xhr.send(formData);
});
