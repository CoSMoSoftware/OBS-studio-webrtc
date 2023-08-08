#!/bin/sh
set -e

echo "Starting the Sphinx Action"

if [ -n "$INPUT_PRE_BUILD_COMMANDS" ]; then
  echo "Execute pre-build commands specified by the user."
  eval "$INPUT_PRE_BUILD_COMMANDS"
fi

if [ -z "${INPUT_TOKEN}" ] && [ "${INPUT_BUILD_ONLY}" != true ]; then
  echo "::error::No token provided. Please set the token parameter."
  exit 1
fi

if [ -n "${INPUT_SPHINX_SRC}" ]; then
  SPHINX_SRC="${INPUT_SPHINX_SRC}"
  echo "::debug::Source directory is set via input parameter"
else
  SPHINX_SRC=$(find . -type d -exec test -e {}/Makefile -a -e {}/conf.py \; -print)
  SPHINX_FILES_COUNT=$(echo "$SPHINX_SRC" | wc -l)
  if [ "$SPHINX_FILES_COUNT" != "1" ]; then
    echo "::error::Found $SPHINX_FILES_COUNT Sphinx sites from $SPHINX_SRC! Please define which to use with input variable \"sphinx_src\""
    exit 1
  fi
  SPHINX_SRC=$(echo $SPHINX_SRC | tr -d '\n')
  echo "::debug::Source directory is found in file system"
fi
echo "::debug::Using \"${SPHINX_SRC}\" as a source directory"

if [ -n "${INPUT_SPHINX_ENV}" ]; then
  echo "::debug::Environment is set via input parameter"
else
  echo "::debug::Environment default in use - production"
  INPUT_SPHINX_ENV="production"
fi

if [ -n "${INPUT_TARGET_BRANCH}" ]; then
  # If a target branch is provided, use it
  remote_branch="${INPUT_TARGET_BRANCH}"
  echo "::debug::target branch is set via input parameter"
else
  # Figure out the GH Pages branch. Only if its a public repo.
  repo_details_response=$(curl -sH "Authorization: token ${INPUT_TOKEN}" \
    "https://api.github.com/repos/${GITHUB_REPOSITORY}")
  is_private=$(echo "$repo_details_response" | awk -F'[:,]' '/\"private\"/ { print $2 }')
  echo "::debug::Is this repo private? ${is_private}"
  if [ -z "${is_private}" ]; then
    echo "::error::Cannot get repository visibility via API."
    echo "::error::${repo_details_response}"
    exit 1
  elif [ $is_private = false ]; then
    # This is a public repo. Getting the Pages settings
    remote_branch_response=$(curl -sH "Authorization: token ${INPUT_TOKEN}" \
                  "https://api.github.com/repos/${GITHUB_REPOSITORY}/pages")
    remote_branch=$(echo "$remote_branch_response" | awk -F'"' '/\"branch\"/ { print $4 }')
    echo "::debug::Pages' branch from settings is ${remote_branch}"

    if [ -z "${remote_branch}" ]; then
      echo "::error::Cannot get GitHub Pages source branch via API."
      echo "::error::${remote_branch_response}"
      exit 1
    fi
  else
    # This is a private repo.
    # Is this a regular repo or an org.github.io type of repo
    case "${GITHUB_REPOSITORY}" in
      *.github.io)
        default_branch=$(echo "$repo_details_response" | awk -F'"' '/\"default_branch\\"/ { print $4 }')
        echo "::debug::Repo default branch is ${default_branch}"

        if [ -z "${default_branch}" ]; then
          echo "::error::Cannot get the default branch via API."
          echo "::error::${default_branch_response}"
          exit 1
        fi
        remote_branch=$default_branch ;;
      *)
        remote_branch="gh-pages" ;;
    esac
  fi
fi

echo "::debug::Remote branch is ${remote_branch}"

REMOTE_REPO="https://${GITHUB_ACTOR}:${INPUT_TOKEN}@github.com/${GITHUB_REPOSITORY}.git"
echo "::debug::Remote is ${REMOTE_REPO}"
BUILD_DIR="${GITHUB_WORKSPACE}/../sphinx_build"
echo "::debug::Build dir is ${BUILD_DIR}"

mkdir $BUILD_DIR
cd $BUILD_DIR

if [ -n "${INPUT_TARGET_PATH}" ] && [ "${INPUT_TARGET_PATH}" != '/' ]; then
  TARGET_DIR="${BUILD_DIR}/${INPUT_TARGET_PATH}"
  echo "::debug::target path is set to ${INPUT_TARGET_PATH}"
else
  TARGET_DIR=$BUILD_DIR
fi

if [ "${INPUT_KEEP_HISTORY}" = true ]; then
  echo "::debug::Cloning ${remote_branch} from repo ${REMOTE_REPO}"
  git clone --branch $remote_branch $REMOTE_REPO .
  LOCAL_BRANCH=$remote_branch
  PUSH_OPTIONS=""
  COMMIT_OPTIONS="--allow-empty"
else
  echo "::debug::Initializing new repo"
  LOCAL_BRANCH="master"
  git init
  PUSH_OPTIONS="--force"
  COMMIT_OPTIONS=""
fi

echo "::debug::Local branch is ${LOCAL_BRANCH}"

cd "${GITHUB_WORKSPACE}"

echo "::debug::Checking for requirements file in ${GITHUB_WORKSPACE}/requirements.txt"
if [ -f "${GITHUB_WORKSPACE}/requirements.txt" ]; then
    echo "::debug::Installing Project requirements"
    pip install -r ${GITHUB_WORKSPACE}/requirements.txt
fi

echo "::debug::Checking for poetry file in ${GITHUB_WORKSPACE}/pyproject.toml"
if [ -f "${GITHUB_WORKSPACE}/pyproject.toml" ]; then
    echo "::debug::Installing Poetry Project requirements"
    poetry build -f wheel
    pip install dist/*.whl
fi

echo "::debug::Checking for requirements file in ${SPHINX_SRC}/requirements.txt"
if [ -f "${SPHINX_SRC}/requirements.txt" ]; then
    echo "::debug::Installing Sphinx requirements"
    pip install -r ${SPHINX_SRC}/requirements.txt
fi

VERBOSE=""
if [ "${SPHINX_DEBUG}" = true ]; then
  # Activating debug for Sphinx
  echo "::debug::Sphinx debug is on"
  VERBOSE="-v"
else
  echo "::debug::Sphinx debug is off"
fi

SPHINX_ENV=${INPUT_SPHINX_ENV} sphinx-build ${SPHINX_SRC} ${TARGET_DIR} ${INPUT_SPHINX_BUILD_OPTIONS} ${VERBOSE}
echo "Sphinx build done"

if [ "${INPUT_BUILD_ONLY}" = true ]; then
  exit $?
fi

if [ "${GITHUB_REF}" = "refs/heads/${remote_branch}" ]; then
  echo "::error::Cannot publish on branch ${remote_branch}"
  exit 1
fi

cd ${BUILD_DIR}

if [ -n "${INPUT_CNAME}" ]; then
  echo $INPUT_CNAME > CNAME
fi

# No need to have GitHub Pages to run Jekyll
touch .nojekyll

echo "Publishing to ${GITHUB_REPOSITORY} on branch ${remote_branch}"

git config user.name "${GITHUB_ACTOR}" && \
git config user.email "${GITHUB_ACTOR}@users.noreply.github.com" && \
git add . && \
git commit $COMMIT_OPTIONS -m "Sphinx build from Action ${GITHUB_SHA}" && \
git push $PUSH_OPTIONS $REMOTE_REPO $LOCAL_BRANCH:$remote_branch && \
rm -fr .git && \
cd ..

exit $?
