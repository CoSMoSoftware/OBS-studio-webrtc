.. raw:: html

   <h1 align="center">

Sphinx Publish Action: Publish Sphinx to Github Pages, Quick and Easy.

.. raw:: html

   </h1>

   <p align="center">


`About`_ • `Usage`_ • `Inputs`_ • `Author`_ • `Support`_ • `Donate`_ • `License`_


.. raw:: html

   </p>

   <p align="center">

.. raw:: html

   </p>

--------------

*****
About
*****

.. raw:: html

   <table>
   <tr>
   <td>

**sphinx-publish-action** is a GitHub Action to build and publish Sphinx sites to GitHub Pages

Remember that GitHub is serving your built static site, not its sources. So when
configuring GitHub Pages in your project settings, use **gh-pages branch** as a
Source for GitHub Pages. If you are setting up *username*.github.io repository,
you'll have to use **master branch**, so sources can be located in another orphaned
branch in the repo (which you can safely mark as default after the first publication).
In addition to that default behaviour, you can configure the branch this plugin pushes
into with the `target_branch`-option. Keep in mind to set the source branch accordingly
at the GitHub Pages Settings page.

.. raw:: html

   </td>
   </tr>
   </table>

*****
Usage
*****

Create a Sphinx project
=======================
If your repo doesn't already have one, create a new Sphinx project:

.. code:: bash
    sphinx-quickstart

See [the Sphinx website](https://www.sphinx-doc.org/en/master/usage/quickstart.html)
for more information. In this repo, we have created a site within a `sample_site`
folder within the repository because the repository's main goal is not to be a
website. If it was the case, we would have created the site at the root of the
repository.


Configure you Sphinx project
============================
Edit the conf,oy file to leverage plugins. In our sample, we are using read the docs
theme, along with some other extensions for reading docstrings in python.

Use the action
==============
Use the `totaldebug/action-sphinx@master` action in your workflow file. It needs
access to the out-of-the-box `GITHUB_TOKEN` secret. The directory where the Sphinx
project lives will be detected (based on the location of `conf.py` and `Makefile`)
but you can also explicitly set this directory by setting the `sphinx_src` parameter
(`sample_site` for us).

.. code:: yaml

    name: Testing the GitHub Pages publication

    on:
        push:

    jobs:
        jekyll:
            runs-on: ubuntu-latest
            steps:
            - uses: actions/checkout@v2

            # Standard usage
            - uses:  totaldebug/action-sphinx@v2
            with:
                token: ${{ secrets.GITHUB_TOKEN }}

            # Specify the Sphinx source location as a parameter
            - uses: totaldebug/action-sphinx@v2
            with:
                token: ${{ secrets.GITHUB_TOKEN }}
                sphinx_src: 'sample_site'

            # Specify the target branch (optional)
            - uses: totaldebug/action-sphinx@v2
            with:
                token: ${{ secrets.GITHUB_TOKEN }}
                target_branch: 'gh-pages'

Upon successful execution, the GitHub Pages publishing will happen automatically
and will be listed on the *_environment_* tab of your repository.

Just click on the *_View deployment_* button of the `github-pages` environment
to navigate to your GitHub Pages site.

******
Inputs
******

======================  =============================================================
Directive Name          Description (Docutils version added to, in [brackets])
======================  =============================================================
token                   the `GITHUB_TOKEN` secret. This is mandatory unless
                        `build_only` is set to `true`.
sphinx_env              The Sphinx environment to build (default to `production`)
sphinx_src              The Sphinx website source directory
sphinx_build_options    Additional Sphinx build arguments
target_branch           The target branch name the sources get pushed to
target_path             The relative path where the site gets pushed to
build_only              When set to `true`, the Sphinx site will be built but not
                        published
pre_build_commands      Commands to run prior to build and deploy. Useful for
                        ensuring build dependencies are up to date or installing
                        new dependencies.
cname                   Contents of CNAME file, this will be copied over.
======================  =============================================================


Example Usage:
=====================

.. code:: python
   name: Testing the GitHub Pages publication

   on:
      push

   jobs:
     sphinx:
       runs-on: ubuntu-latest
       steps:
       - uses: actions/checkout@v2

       # Standard usage
       - uses:  totaldebug/sphinx-publish-action@v2
         with:
           token: ${{ secrets.GITHUB_TOKEN }}

       # Specify the sphinx source location as a parameter
       - uses: totaldebug/sphinx-publish-action@v2
         with:
           token: ${{ secrets.GITHUB_TOKEN }}
           sphinx_src: 'sample_site'

       # Specify the target branch (optional)
       - uses: totaldebug/sphinx-publish-action@v2
         with:
           token: ${{ secrets.GITHUB_TOKEN }}
           target_branch: 'gh-pages'


*******
Support
*******

Create a `ACTIONS_STEP_DEBUG` secret with value `true` and run the workflow again.

Reach out to me at one of the following places:

-  `Discord <https://discord.gg/6fmekudc8Q>`__
-  `Discussions <https://github.com/totaldebug/pyarr/discussions>`__
-  `Issues <https://github.com/totaldebug/pyarr/issues/new/choose>`__


******
Donate
******

Please consider supporting this project by sponsoring, or just donating
a little via `our sponsor
page <https://github.com/sponsors/marksie1988>`__


******
Author
******

.. list-table::
   :header-rows: 1

   * - |TotalDebug|
   * - **marksie1988 (Steven Marks)**


*******
License
*******

|License: CC BY-NC-SA 4.0|

-  Copyright © `Total Debug <https://totaldebug.uk>`__.

.. |TotalDebug| image:: https://totaldebug.uk/assets/images/logo.png
   :target: https://linkedin.com/in/marksie1988
   :width: 150
.. |License: CC BY-NC-SA 4.0| image:: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-orange.svg?style=flat-square
   :target: https://creativecommons.org/licenses/by-nc-sa/4.0/
