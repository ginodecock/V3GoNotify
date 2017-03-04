// load the things we need
var mongoose = require('mongoose');

// define the schema for our user model
var sensorlogSchema = mongoose.Schema({
    sensorId: { type: String, required: true},
    value1: { type: Number, required: false},
    value2: { type: Number, required: false},
    value3: { type: Number, required: false},
    status: { type: String, required: true},
    timestamp: { type: Date, required: true},
    //id: { type: Number, required: true, unique: true}
});

// create the model for users and expose it to our app
module.exports = mongoose.model('Sensorlog', sensorlogSchema);
