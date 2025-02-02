//js implementation of OGREnvelope3D class

var Envelope3D = module.exports = function Envelope3D(envelope) {
	if (envelope) {
		this.minX = envelope.minX;
		this.minY = envelope.minY;
		this.minZ = envelope.minZ || 0;
		this.maxX = envelope.maxX;
		this.maxY = envelope.maxY;
		this.maxZ = envelope.maxZ || 0;
	} else {
		this.minX = 0;
		this.minY = 0;
		this.minZ = 0;
		this.maxX = 0;
		this.maxY = 0;
		this.maxZ = 0;
	}
};

Envelope3D.prototype.isEmpty = function() {
	return !(this.minX || this.minY || this.minZ || this.maxX || this.maxY || this.maxZ);
};

Envelope3D.prototype.merge = function(envelope) {
 	if (arguments.length == 1) {
		if (this.isEmpty()) {
			this.minX = envelope.minX;
			this.maxX = envelope.maxX;
			this.minY = envelope.minY;
			this.maxY = envelope.maxY;
			this.minZ = envelope.minZ;
			this.maxZ = envelope.maxZ;
		} else {
			this.minX = Math.min(envelope.minX, this.minX);
			this.maxX = Math.max(envelope.maxX, this.maxX);
			this.minY = Math.min(envelope.minY, this.minY);
			this.maxY = Math.max(envelope.maxY, this.maxY);
			this.minZ = Math.min(envelope.minZ, this.minZ);
			this.maxZ = Math.max(envelope.maxZ, this.maxZ);
		}
	} else {
		var x = arguments[0];
		var y = arguments[1];
		var z = arguments[2];
		if (this.isEmpty()) {
			this.minX = this.maxX = x;
			this.minY = this.maxY = y;
			this.minZ = this.maxZ = z;
		} else {
			this.minX = Math.min(x, this.minX);
			this.maxX = Math.max(x, this.maxX);
			this.minY = Math.min(y, this.minY);
			this.maxY = Math.max(y, this.maxY);
			this.minZ = Math.min(z, this.minZ);
			this.maxZ = Math.max(z, this.maxZ);
		}
	}
};

Envelope3D.prototype.intersects = function(envelope) {
	return this.minX <= envelope.maxX && this.maxX >= envelope.minX &&
	       this.minY <= envelope.maxY && this.maxY >= envelope.minY &&
	       this.minZ <= envelope.maxZ && this.maxZ >= envelope.minZ;
};

Envelope3D.prototype.intersect = function(envelope) {
	if (this.intersects(envelope)) {
		if (this.isEmpty()) {
			this.minX = envelope.minX;
			this.maxX = envelope.maxX;
			this.minY = envelope.minY;
			this.maxY = envelope.maxY;
			this.minZ = envelope.minZ;
			this.maxZ = envelope.maxZ;
		} else {
			this.minX = Math.max(envelope.minX, this.minX);
			this.maxX = Math.min(envelope.maxX, this.maxX);
			this.minY = Math.max(envelope.minY, this.minY);
			this.maxY = Math.min(envelope.maxY, this.maxY);
			this.minZ = Math.max(envelope.minZ, this.minZ);
			this.maxZ = Math.min(envelope.maxZ, this.maxZ);
		}
	} else {
		this.minX = this.maxX = this.minY = this.maxY = this.minZ = this.maxZ = 0;
	}
};

Envelope3D.prototype.contains = function(envelope) {
	return this.minX <= envelope.minX && this.minY <= envelope.minY && this.minZ <= envelope.minZ &&
	       this.maxX >= envelope.maxX && this.maxY >= envelope.maxY && this.maxZ >= envelope.maxZ;
};