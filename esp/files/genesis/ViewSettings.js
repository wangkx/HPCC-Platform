define([
	"dojo/_base/declare", "dojo/_base/fx", "dojo/dom", "dojo/dom-style", "dojo/dom-geometry", 
	"dojo/parser", "dijit/_WidgetBase", "dijit/_TemplatedMixin", 
	"dojo/ready", "dijit/registry"
	], function(declare, fx, dom, domStyle, domGeometry, parser, 
	_WidgetBase, _TemplatedMixin, ready, registry) {
		svnpath = "";
		
		var initUi = function () {
			var _svnpath = dojo.queryToObject(dojo.doc.location.search.substr((dojo.doc.location.search.substr(0, 1) == "?" ? 1 : 0)))["SVNPath"];
			if (_svnpath) {
				svnpath = _svnpath;
			}
			// set up the store to get the tree data, plus define the method
			// to query the children of a node
			registry.byId("appLayout").init();
		},

		startLoading = function (targetNode) {
			var overlayNode = dom.byId("loadingOverlay");
			if ("none" == domStyle.get(overlayNode, "display")) {
				var coords = domGeometry.getMarginBox(targetNode || baseWindow.body());
				domGeometry.setMarginBox(overlayNode, coords);
				domStyle.set(dom.byId("loadingOverlay"), {
					display: "block",
					opacity: 1
				});
			}
		},

		endLoading = function () {
			fx.fadeOut({
				node: dom.byId("loadingOverlay"),
				duration: 175,
				onEnd: function (node) {
					domStyle.set(node, "display", "none");
				}
			}).play();
		}

    return {
        init: function () {
            startLoading();
            ready(function () {
                initUi();
                endLoading();
            });
        }
    };
});