// load the things we need
var mongoose = require('mongoose'),
	Schema = mongoose.Schema,
	ObjectId = Schema.ObjectId;
// define the schema for our user model
var sensorSchema = mongoose.Schema({
    sensorId: { type: String, required: true },
    name: { type: String, required: true },
    type: { type: String, required: true },
    pbid: { type: String, required: false},
    pbnotify: {type: Boolean, required: false},
    timestamp: { type: Date, required: false},
    user_id: {type: ObjectId},
    rebootoffset: { type: Number, default: 0},
    meteroffset: { type: Number, default: 0}
});
module.exports = mongoose.model('Sensor', sensorSchema);
