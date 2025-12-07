FROM quay.io/fedora/fedora-minimal:42

# No pulling optional dependencies
RUN echo install_weak_deps=False >> /etc/dnf/dnf.conf && \
    dnf install -y git sudo openssh-server nano wget tar bash-completion xauth clang make glfw-devel glslang vulkan-tools vulkan-loader-devel mesa-vulkan-drivers vulkan-validation-layers-devel libXxf86vm-devel --skip-unavailable && \
    dnf clean all && rm -rf /var/cache/dnf

# Manually install eza
RUN wget https://github.com/eza-community/eza/releases/download/v0.21.5/eza_x86_64-unknown-linux-gnu.tar.gz -O eza.tar.gz && \
    tar -xvf eza.tar.gz && \
    mv eza /usr/bin && \
    rm eza.tar.gz

# Create a user for ssh
RUN useradd -m dev && \
# Passwordless sudo privilege
    echo "dev ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/dev && \
    chmod 0440 /etc/sudoers.d/dev