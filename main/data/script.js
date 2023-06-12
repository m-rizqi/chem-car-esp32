function setDate(date){
    document.getElementById("date").innerHTML = date;
}

function setCurrentTime(currentTime){
    document.getElementById("currentTime").innerHTML = currentTime + "WIB";
}

function setTime(time){
    document.getElementById("time").innerHTML = time;
}

function setDistance(distance){
    document.getElementById("distance").innerHTML = distance;
}

function setAcceleration(acceleration){
    document.getElementById("acceleration").innerHTML = acceleration;
}

function setSpeedGauge(value){
    const arc = document.querySelector("svg path");
    const arc_length = arc.getTotalLength();
    arc.style.strokeDasharray = `${value} ${arc_length}`;

    document.getElementById("speed").innerHTML = value;
}

var speedChart;
function setSpeedChart(labels, data){
    const speed_ctx = document.getElementById("speed_chart");
    speedChart = new Chart(speed_ctx, {
        type: "line",
        data: {
        labels: labels,
        datasets: [
            {
            data: data,
            label: "Speed(m/s)",
            borderColor: "rgb(22, 93,255)",
            backgroundColor: "rgb(22, 93,255,0.1)",
            },
        ],
        },
    });
}

function appendNewSpeed(label, data) {
    setSpeedGauge(data);
    speedChart.data.labels.push(label);
    speedChart.data.datasets[0].data.push(data);
    speedChart.update();
}

function setTemperature(data){
    document.getElementById("temperature-value").innerHTML = data + "°";

    const styleSheet = Array.from(document.styleSheets).find(
        sheet => sheet.href === null || sheet.href.startsWith(window.location.origin)
      ); 
    let ruleIndex = -1;
    const rules = styleSheet.cssRules || styleSheet.rules;
    for (let i = 0; i < rules.length; i++) {
        if (rules[i].selectorText === "#temperature-bar::after") {
            ruleIndex = i;
            break;
        }
    }
    if (ruleIndex !== -1) {
        rules[ruleIndex].style.height = (360 / 100 * data) + "px";
    }

}

var temperatureChart;
function setTemperatureChart(labels, data){
    const temperature_ctx = document.getElementById("temperature_chart");
    temperatureChart = new Chart(temperature_ctx, {
        type: "line",
        data: {
        labels: labels,
        datasets: [
            {
            data: data,
            label: "Temperature (Celcius)",
            borderColor: "rgb(248, 122, 83)",
            backgroundColor: "rgb(248, 122, 83,0.1)",
            },
        ],
        },
    });

}

function appendNewTemperature(label, data) {
    setTemperature(data);
    temperatureChart.data.labels.push(label);
    temperatureChart.data.datasets[0].data.push(data);
    temperatureChart.update();
}

function setHumidity(data){
    document.getElementById("humidity-value").innerHTML = data + "%"
    const humidity_doughnut_ctx = document.getElementById("humidity_doughnut");
    var humidity_doughnut = new Chart(humidity_doughnut_ctx, {
        type: "doughnut",
        data: {
            labels: ['Humidity'],
            datasets: [{
                data: [data, (100 - data)],
                backgroundColor: [
                'rgb(0, 197, 152)',
                'rgb(255, 255, 255, 0)',
                ],
                hoverOffset: 4
            }],
        },
        options: {
            cutoutPercentage: 80,
            legend: {
                display: false
            }
        }
    });

}

var humidityChart;
function setHumidityChart(labels, data){
    const humidity_chart_ctx = document.getElementById("humidity_chart");
    humidityChart = new Chart(humidity_chart_ctx, {
        type: "line",
        data: {
        labels: labels,
        datasets: [
            {
            data: data,
            label: "Humidity",
            borderColor: "rgb(0, 197, 152)",
            backgroundColor: "rgb(0, 197, 152,0.1)",
            },
        ],
        },
    });
}

function appendNewHumidity(label, data) {
    setHumidity(data);
    humidityChart.data.labels.push(label);
    humidityChart.data.datasets[0].data.push(data);
    humidityChart.update();
}

var angle = 0;
var animationFrame;
function setTilt(tilt) {
  var steer = document.getElementById("steer");
  document.getElementById("tilt-value").innerHTML = tilt + "°";
  var tiltDirection = document.getElementById("tilt-direction");

  if (tilt < 0) {
    tiltDirection.innerHTML = "Left";
  } else if (tilt === 0) {
    tiltDirection.innerHTML = "Straight";
    tiltDirection.style.color = "green";
  } else {
    tiltDirection.innerHTML = "Right";
  }

  cancelAnimationFrame(animationFrame); // Cancel any previous animation frame

  var duration = 1000; // Animation duration in milliseconds
  var startAngle = angle;
  var changeAngle = tilt - startAngle;
  var startTime = null;

  function animate(currentTime) {
    if (!startTime) startTime = currentTime;
    var elapsedTime = currentTime - startTime;
    var progress = Math.min(elapsedTime / duration, 1);
    var easing = easeInOutQuad(progress);
    angle = startAngle + easing * changeAngle;
    steer.style.transform = `rotate(${angle}deg)`;

    if (progress < 1) {
      animationFrame = requestAnimationFrame(animate);
    }
  }

  animationFrame = requestAnimationFrame(animate);
}
// Easing function for smooth animation
function easeInOutQuad(t) {
  t /= 0.5;
  if (t < 1) return 0.5 * t * t;
  t--;
  return -0.5 * (t * (t - 2) - 1);
}

const canvas = document.getElementById("road-canvas");
const context = canvas.getContext("2d");

function scaleCoordinateValue(coordinate, canvasWidth, canvasHeight, xRange, yRange){
    var xScaleFactor = canvasWidth / xRange;
    var yScaleFactor = canvasHeight / yRange;
    var xScaled = coordinate[0] * xScaleFactor;
    var yScaled = coordinate[1] * yScaleFactor + (yRange/2 * yScaleFactor);
    return [xScaled, yScaled];
}

function setCarPosition(xScaled, yScaled, canvasWidth, canvasHeight){
    const carElement = document.getElementById("car");
    var topVal = yScaled / canvasHeight * 100;
    var leftVal = (xScaled / canvasWidth * 100);
    carElement.style.top = topVal + "%";
    carElement.style.left = leftVal + "%";
}

function drawCarMovement(coordinates) {
    var canvas = document.getElementById("road-canvas");
    var context = canvas.getContext("2d");

    var canvasWidth = canvas.width;
    var canvasHeight = canvas.height;
    var xMax = canvasWidth;
    var yMax = canvasHeight;
    var xRange = 12;
    var yCoordMax = Math.max(...coordinates.map(coord => coord[1]));
    var yCoordMin = Math.min(...coordinates.map(coord => coord[1]));
    // var yRange = yCoordMax - yCoordMin;
    var yRange = 8;

    context.beginPath();
    var [x, y] = scaleCoordinateValue(coordinates[0], canvasWidth, canvasHeight, xRange, yRange);
    context.moveTo(x, y);
    
    for (var i = 1; i < coordinates.length; i++) {
        [x, y] = scaleCoordinateValue(coordinates[i], canvasWidth, canvasHeight, xRange, yRange);
        context.lineTo(x, y);
        setCarPosition(x, y, canvasWidth, canvasHeight);
    }
    
    context.strokeStyle = "red";
    context.lineWidth = 2;
    context.stroke();
}
