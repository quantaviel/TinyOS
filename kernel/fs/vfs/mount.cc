#include <fs/vfs.h>
#include <klib/string.h>

namespace vfs {

#define MOUNT_SIZE 128

mount_node m_nodes[MOUNT_SIZE];
mount_node* root_mount;

void mount_lock() {}
void mount_unlock() {}

int mount(mount_node* parent, super_block* sb, dentry* root, dentry* mount_point) {
    if (sb == nullptr || root == nullptr) {
        return -1;
    }
    if (root->in == nullptr) {
        return -1;
    }
    if ((parent == nullptr && mount_point != nullptr) || (parent != nullptr && mount_point == nullptr)) {
        return -1;
    }

    mount_node* node {};
    mount_lock();
    for (int i = 0; i < MOUNT_SIZE; ++i) {
        if (m_nodes[i].state == mount_node_state::used && m_nodes[i].mount_point == mount_point) {
            // Multiple mount on same point: error
            /// FUTURE: Finish the conditions
            mount_unlock();
            return -1;
        }
    }
    for (int i = 0; i < MOUNT_SIZE; ++i) {
        if (m_nodes[i].state == mount_node_state::avaliable) {
            m_nodes[i].mount_point = mount_point;
            m_nodes[i].parent = parent;
            m_nodes[i].root = root;
            m_nodes[i].sb = sb;
            m_nodes[i].state = mount_node_state::used;
            node = &m_nodes[i];
            break;
        }
    }
    mount_unlock();

    if (node == nullptr) {
        return -1;
    }

    if (parent == nullptr && mount_point == nullptr) {
        root_mount = node;
        root_inode = root->in;
    }
    return 1;
}

int unmount(mount_node* node) {
    /// FUTURE: Need to complete
}

} // namespace vfs