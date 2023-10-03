FROM ubuntu:22.04

RUN apt update && apt install g++ cmake ninja-build -y
RUN mkdir code
COPY run.sh /

EXPOSE 5433

CMD ["/bin/bash", "/run.sh"]