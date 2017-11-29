import datetime
import json
import requests
import time

t1 = "10"
p1 = "70"
h1 = "20"
r1 = "900"


url='http://138.197.23.70:8080/station/reading'
headers = {'Content-type': 'application/json', 'Accept': 'text/plain'}

data = {
  "temperature": int(t1),
  "pressure": 60.0,
  "humidity": 800.0,
  "radiation": 50.0,
  "timestamp": datetime.datetime.now().isoformat(), 
}
#print data
response = requests.post(url, data=json.dumps(data), headers=headers)
try:
  assert response.status_code is 201
except AssertionError:
    raise AssertionError("Error in request", response)
