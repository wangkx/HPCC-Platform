/*##############################################################################
#    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
############################################################################## */
define([
    "dojo/_base/declare",
    "dojo/_base/lang",
    "dojo/i18n",
    "dojo/i18n!./nls/hpcc",
    "dojo/dom",
    "dojo/request/xhr",

    "dijit/layout/BorderContainer",
    "dijit/layout/ContentPane",
    "dijit/registry",

    "hpcc/_Widget",
    "hpcc/ESPRequest",
    "hpcc/ESPWorkunit",
    "hpcc/ESPDFUWorkunit",
    "hpcc/WsTopology",

    "dojo/text!../templates/XMLContentWidget.html",

    "dijit/Toolbar",
    "dijit/ToolbarSeparator",
    "dijit/form/Button"

], function (declare, lang, i18n, nlsHPCC, dom, xhr,
            BorderContainer, ContentPane, registry,
            _Widget, ESPRequest, ESPWorkunit, ESPDFUWorkunit, WsTopology,
            template) {
        return declare("XMLContentWidget", [_Widget], {
            templateString: template,
            baseClass: "XMLContentWidget",
            i18n: nlsHPCC,

            borderContainer: null,
            eclSourceContentPane: null,
            wu: null,
            editor: null,
            markers: [],
            highlightLines: [],
            readOnly: false,

            buildRendering: function (args) {
                this.inherited(arguments);
            },

            postCreate: function (args) {
                this.inherited(arguments);
                this.borderContainer = registry.byId(this.id + "BorderContainer");
            },

            startup: function (args) {
                this.inherited(arguments);
            },

            resize: function (args) {
                this.inherited(arguments);
                this.borderContainer.resize();
                if (this.editor) {
                    this.editor.setSize("100%", "100%");
                }
            },

            layout: function (args) {
                this.inherited(arguments);
            },

            //  Plugin wrapper  ---
            init: function (params) {
                if (this.inherited(arguments))
                    return;

                if (params.readOnly !== undefined)
                    this.readOnly = params.readOnly;

                this.editor = CodeMirror.fromTextArea(document.getElementById(this.id + "XML"), {
                    tabMode: "indent",
                    matchBrackets: true,
                    lineNumbers: true,
                    mode: "xml",
                    readOnly: this.readOnly,
                    foldGutter: true,
                    gutters: ["CodeMirror-linenumbers", "CodeMirror-foldgutter"]
                });
                dom.byId(this.id + "XMLContent").style.backgroundColor = this.readOnly ? 0xd0d0d0 : 0xffffff;
                this.editor.setSize("100%", "100%");

                var context = this;
                if (params.SourceType === "ECLWU") {
                    this.wu = ESPWorkunit.Get(params.Wuid);
                    this.wu.fetchXML(function (response) {
                        var xml = context.formatXml(response);
                        context.setText(xml);
                    });
                } else if (params.SourceType === "DFUWU") {
                    this.wu = ESPDFUWorkunit.Get(params.Wuid);
                    this.wu.fetchXML(function (response) {
                        var xml = context.formatXml(response);
                        context.setText(xml);
                    });
                } else if (params.SourceType === "Server") {
                    WsTopology.TpGetComponentFile({
                        request: {
                            CompType: params.CompType,
                            CompName: params.CompName,
                            NetAddress: params.NetAddress,
                            Directory: params.Directory,
                            FileType: "cfg",
                            OsType: params.OsType
                        }
                    }).then(function (response) {
                        var xml = context.formatXml(response);
                        context.setText(xml);
                    });
                } else {
                    ESPRequest.send("main", "", {
                        request: {
                            config_: "",
                            PlainText: "yes"
                        },
                        handleAs: "text"
                    }).then(function(response) {
                        var xml = context.formatXml(response);
                        context.setText(xml);
                    }); 
                }
            },

            clearErrors: function (errWarnings) {
                for (var i = 0; i < this.markers.length; ++i) {
                    this.editor.clearMarker(this.markers[i]);
                }
                this.markers = [];
            },

            setErrors: function (errWarnings) {
                for (var i = 0; i < errWarnings.length; ++i) {
                    this.markers.push(this.editor.setMarker(parseInt(
                            errWarnings[i].LineNo, 10) - 1, "",
                            errWarnings[i].Severity + "Line"));
                }
            },

            setCursor: function (line, col) {
                this.editor.setCursor(line - 1, col - 1);
                this.editor.focus();
            },

            clearHighlightLines: function () {
                for (var i = 0; i < this.highlightLines.length; ++i) {
                    this.editor.setLineClass(this.highlightLines[i], null, null);
                }
            },

            highlightLine: function (line) {
                this.highlightLines.push(this.editor.setLineClass(line - 1, "highlightline"));
            },

            setText: function (text) {
                try {
                    this.editor.setValue(text);
                } catch (e) {
                    topic.publish("hpcc/brToaster", {
                        Severity: "Error",
                        Source: "ECLSourceWidget.setText",
                        Exceptions: [
                            { Message: this.i18n.SetTextError },
                            { Message: e.toString ? (this.i18n.Details + ":\n" + e.toString()) : e }
                        ]
                    });
                }
            },

            setReadOnly: function (readonly) {
                this.editor.readOnly(readonly);
            },

            getText: function () {
                return this.editor.getValue();
            }

        });
    });
