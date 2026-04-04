# Configuration file for the Sphinx documentation builder.
#
# For all built-in options, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

import os
import sys

sys.path.insert(0, os.path.abspath("."))

# -- Project information -----------------------------------------------------

project = "threshold-eval"
copyright = "2026, Aaron James Long"
author = "Aaron James Long"

release = "1.1.0"

# -- General configuration ---------------------------------------------------

extensions = [
    "breathe",
]

templates_path = ["_templates"]

exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

source_suffix = ".rst"

master_doc = "index"

language = "en"

# -- Options for HTML output -------------------------------------------------

html_theme = "alabaster"
html_theme_options = {
    "description": "Configurable threshold evaluation for safety-critical systems",
    "github_user": "ACIDBURN2501",
    "github_repo": "threshold-eval",
    "github_button": True,
}

html_static_path = ["_static"]

# -- Options for Breathe -----------------------------------------------------

breathe_default_project = "threshold_eval"
breathe_default_domain = "c"
