
#include "remote_control.h"

using namespace ray::network;

void remote_control_client::async_launch(std::string_view server_full_addr){
}


void remote_control_client::wait_shutdown() {
}


std::optional<remote_command_frame_set> remote_control_client::receive_next_frame_command(){
        return std::nullopt;
}


void remote_control_client::send_answer(const remote_answer_frame_set &command_answer){
}