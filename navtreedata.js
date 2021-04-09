/*
@licstart  The following is the entire license notice for the
JavaScript code in this file.

Copyright (C) 1997-2019 by Dimitri van Heesch

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

@licend  The above is the entire license notice
for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Garlic Model", "index.html", [
    [ "Introduction", "index.html", [
      [ "What is this?", "index.html#autotoc_md34", [
        [ "1. Data Container Protocol", "index.html#autotoc_md35", null ],
        [ "2. Schema Validation to validate data containers", "index.html#autotoc_md36", null ],
        [ "3. Encoding Utilities to serialize and deserialize custom C++ classes and types.", "index.html#autotoc_md37", null ]
      ] ],
      [ "What's missing?", "index.html#autotoc_md38", null ],
      [ "How do I play with this?", "index.html#autotoc_md39", null ],
      [ "Performance Tests", "index.html#autotoc_md40", null ]
    ] ],
    [ "Encoding and Decoding", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html", [
      [ "Encoding", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md1", null ],
      [ "Decoding", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md4", null ]
    ] ],
    [ "Layer / Data Container Protocol", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Layer.html", [
      [ "What's the benefit?", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Layer.html#autotoc_md16", [
        [ "View", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Layer.html#autotoc_md17", null ],
        [ "Reference", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Layer.html#autotoc_md18", null ],
        [ "Document and Value", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Layer.html#autotoc_md19", null ]
      ] ]
    ] ],
    [ "Module Parsing", "md__home_runner_work_garlic-models_garlic-models_docs_pages_ModuleParsing.html", null ],
    [ "Validation", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Validation.html", [
      [ "Why not use JSON Schema?", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Validation.html#autotoc_md27", null ],
      [ "Constraint", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Validation.html#autotoc_md28", null ],
      [ "Field", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Validation.html#autotoc_md29", null ],
      [ "Model", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Validation.html#autotoc_md30", [
        [ "Automatic Fields", "md__home_runner_work_garlic-models_garlic-models_docs_pages_ModuleParsing.html#autotoc_md21", null ],
        [ "Deferred Fields", "md__home_runner_work_garlic-models_garlic-models_docs_pages_ModuleParsing.html#autotoc_md22", null ],
        [ "Model Inheritance", "md__home_runner_work_garlic-models_garlic-models_docs_pages_ModuleParsing.html#autotoc_md23", [
          [ "1. Provide an encode function in a specialization of garlic::coder<T> like below:", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md2", null ],
          [ "2. static encoder function for custom classes:", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md3", null ],
          [ "1. Provide a decode function in a specialization of garlic::coder<T> like below:", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md5", null ],
          [ "2. Provide a static decoder function for custom classes:", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md6", null ],
          [ "3. Provide a layer constructor in custom classes.", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md7", null ],
          [ "Safe Decoding", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md8", [
            [ "1. Provide a safe_decode function in a specialization of garlic::coder<T> like below:", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md9", null ],
            [ "2. Provide a static safe_decoder function for custom classes:", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md10", null ]
          ] ],
          [ "Recommendations", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md11", [
            [ "For custom classes, it's more clean and easy to understand to just provide the static encode and decode functions:", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md12", null ],
            [ "Use GARLIC_VIEW and GARLIC_REF instead of typename:", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md13", null ],
            [ "Be careful using garlic::get()", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Encoding.html#autotoc_md14", null ]
          ] ],
          [ "Exclude some fields", "md__home_runner_work_garlic-models_garlic-models_docs_pages_ModuleParsing.html#autotoc_md24", null ],
          [ "Override fields", "md__home_runner_work_garlic-models_garlic-models_docs_pages_ModuleParsing.html#autotoc_md25", null ]
        ] ],
        [ "Model and Field Tags", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Validation.html#autotoc_md31", null ],
        [ "Module", "md__home_runner_work_garlic-models_garlic-models_docs_pages_Validation.html#autotoc_md32", null ]
      ] ]
    ] ],
    [ "Data Structures", "annotated.html", [
      [ "Data Structures", "annotated.html", "annotated_dup" ],
      [ "Data Structure Index", "classes.html", null ],
      [ "Data Fields", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
".html",
"utility_8h.html#ad0889f1349f9272adf6e6ccaa29ce2af"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';