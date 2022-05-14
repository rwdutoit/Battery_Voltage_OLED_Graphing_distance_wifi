const static char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>

<body>

  <div id="demo">
    <h1>The ESP8266 NodeMCU + HC-SR04</h1>
  </div>
  <div>
    Battery Voltage is : <span id="ADCValue">0</span> <button type="button" onclick="getVoltageData()">Get ADC value</button><br>
    Battery Temperature is : <span id="TemperatureValue">0</span> <button type="button" onclick="getTemperatureData()">Get ADC value</button><br>
    Sampling Time : <span id="SamplingTimeValue">0</span> <br>
    Voltage difference is : <span id="VoltageDiffValue">0</span> <button type="button" onclick="getVoltageDiff()">Get Vdiff value</button> <br>
    Battery minutes remaining : <span id="BatteryRemaining">0</span><br>
  </div>

  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://code.highcharts.com/highcharts.js"></script>
  <div id="voltage-chart" class="container"></div>
  <div id="temperature-chart" class="container"></div>
  <div id="difference-chart" class="container"></div>

  <div>
    <form action="/action_page">
      Alert Distance (cm): <input type="text" name="inputAlertDistance">
      <input type="submit" value="Submit">
    </form><br>
  </div>
  <div>
    <form action="/action_page">
      Alert Voltage (V): <input type="text" name="inputAlertVoltage">
      <input type="submit" value="Submit">
    </form><br>
  </div>
  <div>
    <form action="/action_page">
      Voltage difference average alpha: <input type="text" name="movingAlpha">
      <input type="submit" value="Submit">
    </form><br>
  </div>
  <div>
    <form action="/action_page">
      Distance loop delay (ms): <input type="text" name="distanceLoopDelay">
      <input type="submit" value="Submit">
    </form><br>
  </div>
  <div>
    <form action="/action_page">
      Voltage loop delay (ms): <input type="text" name="voltageLoopDelay">
      <input type="submit" value="Submit">
    </form><br>
  </div>
  <script>

    setInterval(function () {
      // Call a function repetatively with 2 Second interval
      getVoltageData();
      getSamplingTime();
      getRemainingBattSeconds();
      getVoltageDiff();
      getTemperatureData();
    }, 2500); //1000mSeconds update rate

    function getSamplingTime() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("SamplingTimeValue").innerHTML = this.responseText;
        }
      };
      xhttp.open("GET", "readSamplingTime", true);
      xhttp.send();
    }

    function getRemainingBattSeconds() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          BatteryRemaining = parseFloat(this.responseText);
          document.getElementById("BatteryRemaining").innerHTML = BatteryRemaining/60;
        }
      };
      xhttp.open("GET", "readRemainingBattSeconds", true);
      xhttp.send();
    }

    function getVoltageData() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("ADCValue").innerHTML = this.responseText;
          var x = (new Date()).getTime(),
            y = parseFloat(this.responseText);
          //console.log(this.responseText);
          if (chartVoltage.series[0].data.length > 40) {
            chartVoltage.series[0].addPoint([x, y], true, true, true);
          } else {
            chartVoltage.series[0].addPoint([x, y], true, false, true);
          }
        }
      };
      xhttp.open("GET", "readADC", true);
      xhttp.send();
    }

    function getTemperatureData() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("TemperatureValue").innerHTML = this.responseText;
          var x = (new Date()).getTime(),
            y = parseFloat(this.responseText);
          //console.log(this.responseText);
          if (chartTemperature.series[0].data.length > 40) {
            chartTemperature.series[0].addPoint([x, y], true, true, true);
          } else {
            chartTemperature.series[0].addPoint([x, y], true, false, true);
          }
        }
      };
      xhttp.open("GET", "readTemperature", true);
      xhttp.send();
    }

    function getVoltageDiff() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("VoltageDiffValue").innerHTML = this.responseText;
          var x = (new Date()).getTime(),
              y = parseFloat(this.responseText);
          //console.log(this.responseText);
          if(chartVdiff.series[0].data.length > 40) {
            chartVdiff.series[0].addPoint([x, y], true, true, true);
          } else {
            chartVdiff.series[0].addPoint([x, y], true, false, true);
          }
        }
      };
      xhttp.open("GET", "readVoltageDiff", true);
      xhttp.send();
    }


    var chartVoltage = new Highcharts.Chart({
      chart: { renderTo: 'voltage-chart' },
      title: { text: 'ESP8266 HC-SR04' },
      series: [{
        showInLegend: false,
        data: []
      }],
      plotOptions: {
        line: {
          animation: false,
          dataLabels: { enabled: true }
        },
        series: { color: '#059e8a' }
      },
      xAxis: {
        type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Voltage (V)' }
      },
      credits: { enabled: false }
    });

    var chartTemperature = new Highcharts.Chart({
      chart: { renderTo: 'temperature-chart' },
      title: { text: 'ESP8266 HC-SR04' },
      series: [{
        showInLegend: false,
        data: []
      }],
      plotOptions: {
        line: {
          animation: false,
          dataLabels: { enabled: true }
        },
        series: { color: '#059e8a' }
      },
      xAxis: {
        type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Temperature (degC)' }
      },
      credits: { enabled: false }
    });

    var chartVdiff = new Highcharts.Chart({
      chart: { renderTo: 'difference-chart' },
      title: { text: 'ESP8266 HC-SR04' },
      series: [{
        showInLegend: false,
        data: []
      }],
      plotOptions: {
        line: {
          animation: false,
          dataLabels: { enabled: true }
        },
        series: { color: '#059e8a' }
      },
      xAxis: {
        type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Vdiff (Vavg)' }
      },
      credits: { enabled: false }
    });


  </script>
</body>

</html>
)=====";