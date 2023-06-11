function updateDate(date){
    document.getElementById("date").innerHTML = date;
}

function updateCurrentTime(currentTime){
    document.getElementById("currentTime").innerHTML = currentTime + "WIB";
}

function updateTime(time){
    document.getElementById("time").innerHTML = time;
}

function updateDistance(distance){
    document.getElementById("distance").innerHTML = distance;
}

function updateSpeed(speed){
    document.getElementById("speed").innerHTML = speed;
}

function updatePotentiometerGauge(value){
    const arc = document.querySelector("svg path");
    const arc_length = arc.getTotalLength();
    arc.style.strokeDasharray = `${value} ${arc_length}`;

    document.getElementById("potentiometer").innerHTML = value + "°";
}

function updatePotentiometer(labels, data){
    const potentiometer_ctx = document.getElementById("potentiometer_chart");
    var potentiometer_chart = new Chart(potentiometer_ctx, {
        type: "line",
        data: {
        labels: labels,
        datasets: [
            {
            data: data,
            label: "Degree of Angle",
            borderColor: "rgb(22, 93,255)",
            backgroundColor: "rgb(22, 93,255,0.1)",
            },
        ],
        },
    });

    updatePotentiometerGauge(data[data.length - 1]);
}

function updateTemperature(labels, data){
    const temperature_ctx = document.getElementById("temperature_chart");
    var temperature_chart = new Chart(temperature_ctx, {
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

    var lastValue = data[data.length - 1];
    document.getElementById("temperature-value").innerHTML = lastValue + "°";
    

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
        rules[ruleIndex].style.height = (360 / 100 * lastValue) + "px";
    }

}

function updateHumidity(labels, data){
    var lastValue = data[data.length - 1]

    document.getElementById("humidity-value").innerHTML = lastValue + "%"

    const humidity_doughnut_ctx = document.getElementById("humidity_doughnut");
    var humidity_doughnut = new Chart(humidity_doughnut_ctx, {
        type: "doughnut",
        data: {
            labels: ['Humidity'],
            datasets: [{
                data: [lastValue, (100 - lastValue)],
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

    const humidity_chart_ctx = document.getElementById("humidity_chart");
    var humidity_chart = new Chart(humidity_chart_ctx, {
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

function updateTilt(tilt){
    document.getElementById("steer").style.transform = "rotate("+tilt+"deg)";
    document.getElementById("tilt-value").innerHTML = tilt + "°";
    var tiltDirection = document.getElementById("tilt-direction");
    if(tilt < 0){
        tiltDirection.innerHTML = "Left";
    }
    if(tilt == 0){
        tiltDirection.innerHTML = "Straight";
        tiltDirection.style.color = "green";
    }
    if(tilt > 0) {
        tiltDirection.innerHTML = "Right";
    }
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

function updateCarPosition(xScaled, yScaled, canvasWidth, canvasHeight){
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
        updateCarPosition(x, y, canvasWidth, canvasHeight);
    }
    
    context.strokeStyle = "red";
    context.lineWidth = 2;
    context.stroke();
}
