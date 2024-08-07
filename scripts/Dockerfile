FROM centos:7

# use baseurl instead of mirrorlist
RUN sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/*.repo
RUN sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/*.repo
RUN sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/*.repo
RUN yum clean all && yum makecache

RUN yum install -y centos-release-scl
# use baseurl instead of mirrorlist in CentOS-SCLo-scl.repo
RUN sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/*.repo
RUN sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/*.repo
RUN sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/*.repo
RUN yum clean all && yum makecache

RUN yum install -y git dtc devtoolset-11 llvm-toolset-7 bison flex
RUN echo "source /opt/rh/devtoolset-11/enable" >> /etc/profile
RUN echo "source /opt/rh/llvm-toolset-7/enable" >> /etc/profile
