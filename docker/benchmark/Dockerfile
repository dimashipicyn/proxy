FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install postgresql-contrib -y
COPY run.sh /

CMD ["/bin/bash", "/run.sh"]