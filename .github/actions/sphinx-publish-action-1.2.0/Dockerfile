FROM python:3.9.6-buster

LABEL version="1.0.0"
LABEL repository="https://github.com/totaldebug/sphinx-publish-action"
LABEL homepage="https://github.com/totaldebug/sphinx-publish-action"
LABEL maintainer="Steven Marks <marksie1988@github.com>"

# debug
RUN apt update -y
RUN apt upgrade -y
RUN apt install curl git -y

RUN pip install -U sphinx poetry
RUN sphinx-build --version

COPY LICENSE README.rst /

COPY entrypoint.sh /
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
