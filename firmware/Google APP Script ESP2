var SPREADSHEET_ID = '1HjXdats-6j_nTqGa1x98_PCKedIaKza6Vd9QtlCUE9s';
var SHEET_NAME = 'Data_Pergeseran_Sheet';

function doGet(e) {
  return handleRequest(e);
}

function doPost(e) {
  return handleRequest(e);
}

function handleRequest(e) {
  try {
    var params = e.parameter;
    var action = params.action;
    
    if (action === 'getData') {
      return getDataAPI();
    } else {
      // Simpan data baru
      var date = params.date || new Date().toISOString().split('T')[0];
      var time = params.time || new Date().toLocaleTimeString();
      var pitch = parseFloat(params.pitch) || 0;
      var roll = parseFloat(params.roll) || 0;
      var average = parseFloat(params.average) || (Math.abs(pitch) + Math.abs(roll)) / 2;
      
      writeToSheet(date, time, pitch, roll, average);
      
      return ContentService.createTextOutput(JSON.stringify({
        status: 'success',
        message: 'Data saved successfully'
      })).setMimeType(ContentService.MimeType.JSON);
    }
  } catch (error) {
    return ContentService.createTextOutput(JSON.stringify({
      status: 'error',
      message: error.toString()
    })).setMimeType(ContentService.MimeType.JSON);
  }
}

function writeToSheet(date, time, pitch, roll, average) {
  var spreadsheet = SpreadsheetApp.openById(SPREADSHEET_ID);
  var sheet = spreadsheet.getSheetByName(SHEET_NAME) || spreadsheet.insertSheet(SHEET_NAME);
  
  if (sheet.getLastRow() === 0) {
    sheet.appendRow(['Date', 'Time', 'Pitch', 'Roll', 'Average']);
  }
  
  sheet.appendRow([date, time, pitch, roll, average]);
}

function getDataAPI() {
  var spreadsheet = SpreadsheetApp.openById(SPREADSHEET_ID);
  var sheet = spreadsheet.getSheetByName(SHEET_NAME);
  
  if (!sheet || sheet.getLastRow() === 0) {
    return ContentService.createTextOutput(JSON.stringify({
      status: 'success',
      data: []
    })).setMimeType(ContentService.MimeType.JSON);
  }
  
  var data = sheet.getDataRange().getValues();
  var headers = data[0];
  var result = [];
  
  for (var i = 1; i < data.length; i++) {
    var row = data[i];
    var obj = {};
    for (var j = 0; j < headers.length; j++) {
      obj[headers[j].toLowerCase()] = row[j];
    }
    result.push(obj);
  }
  
  return ContentService.createTextOutput(JSON.stringify({
    status: 'success',
    data: result
  })).setMimeType(ContentService.MimeType.JSON);
}
