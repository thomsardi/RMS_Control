window.onload = function() {
  fetchData('/get-device-info', changeDeviceInfoContent);
}

function fetchData(url, callback) {
  fetch(url)
  .then(response => {
    if (!response.ok) {
      throw new Error('Network response was not ok');
    }
    clearTimeout()
    return response.json();
  })
  .then(data => {
    // Handle the response data
    callback(data);
    setTimeout(fetchData, 500, url, callback);
    // console.log(data);
  })
  .catch(error => {
    // Handle errors
    setTimeout(fetchData, 1000, url, callback);
    console.error('Fetch error:', error);
  });
}

function changeDeviceInfoContent(data) {
  if('firmware_version' in data) {
    const spanElement = document.getElementById('firmware-version');
    spanElement.textContent = data['firmware_version'];
  }

  if('device_ip' in data) {
    const spanElement = document.getElementById('device-ip');
    spanElement.textContent = data['device_ip'];
  }

  if('mac_address' in data) {
    const spanElement = document.getElementById('mac-address');
    spanElement.textContent = data['mac_address'];
  }

  if('server_mode' in data) {
    const spanElement = document.getElementById('server-mode');
    spanElement.textContent = data['server_mode'];
  }

  if('ssid' in data) {
    const spanElement = document.getElementById('device-ssid');
    spanElement.textContent = data['ssid'];
  }
}

function onClickReboot() {
  const xhr = new XMLHttpRequest();
  xhr.open("POST", "/restart");
  xhr.setRequestHeader("Content-Type", "application/json");
  xhr.onload = function () {
    if (xhr.status === 200) {
      alert("Success");
    } 
    else {
      console.error("Error:", xhr.status);
    }
  };
  xhr.send(JSON.stringify({"restart" : 1}));
}

function onClickFactoryReset() {
  const xhr = new XMLHttpRequest();
  xhr.open("POST", "/set-factory-reset");
  xhr.setRequestHeader("Content-Type", "application/json");
  xhr.onload = function () {
    if (xhr.status === 200) {
      alert("Success");
    } 
    else {
      console.error("Error:", xhr.status);
    }
  };
  xhr.send(JSON.stringify({"factory_reset" : 1}));
}