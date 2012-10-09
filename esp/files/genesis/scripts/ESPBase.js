define([
	"dojo/_base/declare",
	"dojo/_base/config"
], function (declare, config) {
	return declare(null, {

		constructor: function (args) {
			declare.safeMixin(this, args);
		},

		getParam: function (key) {
			var value = dojo.queryToObject(dojo.doc.location.search.substr((dojo.doc.location.search.substr(0, 1) == "?" ? 1 : 0)))[key];

			if (value)
				return value;
			return config[key];
		},

		getBaseURL: function (service) {
			if (!service) {
				service = "Genesis";
			}
			var serverIP = this.getParam("serverIP");
			if (serverIP)
				return "http://" + serverIP + ":8010/" + service;
			return "/" + service;
		},

		getValue: function (domXml, tagName, knownObjectArrays) {
			var retVal = this.getValues(domXml, tagName, knownObjectArrays);
			if (retVal.length == 0) {
				return null;
			} else if (retVal.length != 1) {
				alert("Invalid length:  " + retVal.length);
			}
			return retVal[0];
		},

		getValues: function (domXml, tagName, knownObjectArrays) {
			var retVal = [];
			var items = domXml.getElementsByTagName(tagName);
			var parentNode = items.length ? items[0].parentNode : null; //  Prevent <Dataset><row><field><row> scenario
			for (var i = 0; i < items.length; ++i) {
				if (items[i].parentNode == parentNode)
					retVal.push(this.flattenXml(items[i], knownObjectArrays));
			}
			return retVal;
		},

		flattenXml: function (domXml, knownObjectArrays) {
			var retValArr = [];
			var retValStr = "";
			var retVal = {};
			for (var i = 0; i < domXml.childNodes.length; ++i) {
				var childNode = domXml.childNodes[i];
				if (childNode.childNodes) {
					if (childNode.nodeName && knownObjectArrays != null && dojo.indexOf(knownObjectArrays, childNode.nodeName) >= 0) {
						retValArr.push(this.flattenXml(childNode, knownObjectArrays));
					} else if (childNode.nodeName == "#text") {
						retValStr += childNode.nodeValue;
					} else if (childNode.childNodes.length == 0) {
						retVal[childNode.nodeName] = null;
					} else {
						var value = this.flattenXml(childNode, knownObjectArrays);
						if (retVal[childNode.nodeName] == null) {
							retVal[childNode.nodeName] = value;
						} else if (dojo.isArray(retVal[childNode.nodeName])) {
							retVal[childNode.nodeName].push(value);
						} else if (dojo.isObject(retVal[childNode.nodeName])) {
							var tmp = retVal[childNode.nodeName];
							retVal[childNode.nodeName] = [];
							retVal[childNode.nodeName].push(tmp);
							retVal[childNode.nodeName].push(value);
						}
					}
				}
			}
			if (retValArr.length)
				return retValArr;
			else if (retValStr.length)
				return retValStr;
			return retVal;
		}
	});
});