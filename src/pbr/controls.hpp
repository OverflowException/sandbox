#pragma once

#define GLM_ENABLE_EXPERIMENTAL 
#include <glm/gtx/quaternion.hpp>
#include "imgui.h"

class Ctrl {
public:
    // model transform
    static glm::vec3 model_euler;
    static void model_control() {
        ImGui::Begin("model");
	    ImGui::SliderAngle("x rotation", &model_euler.x, -180.0f, 180.0f);
	    ImGui::SliderAngle("y rotation", &model_euler.y, -180.0f, 180.0f);
	    ImGui::SliderAngle("z rotation", &model_euler.z, -180.0f, 180.0f);
	    ImGui::End();
    }

    // camera
	static glm::vec3 eye;
	static glm::vec3 up;
	static glm::vec3 right;
	static glm::vec3 front;
    static void camera_control() {
        ImGui::Begin("camera roam");
        float speed = 0.003;
		ImGui::Button("Left");
		if(ImGui::IsItemActive()) {
			eye -= right * glm::vec3(speed);
		}
		ImGui::Button("Right");
		if(ImGui::IsItemActive()) {
			eye += right * glm::vec3(speed);
		}
		ImGui::Button("Up");
		if(ImGui::IsItemActive()) {
			eye += up * glm::vec3(speed);
		}
		ImGui::Button("Down");
		if(ImGui::IsItemActive()) {
			eye -= up * glm::vec3(speed);
		}
		ImGui::Button("Forward");
		if(ImGui::IsItemActive()) {
			eye += front * glm::vec3(speed);
		}
		ImGui::Button("Backward");
		if(ImGui::IsItemActive()) {
			eye -= front * glm::vec3(speed);
		}
		ImGui::End();

        ImGui::Begin("look");
        speed = 0.006;
		ImGui::Button("Left");
		if(ImGui::IsItemActive()) {
			right = glm::rotate(glm::angleAxis(speed, up), right);
		}
		ImGui::Button("Right");
		if(ImGui::IsItemActive()) {
			right = glm::rotate(glm::angleAxis(-speed, up), right);
		}
		ImGui::Button("Up");
		if(ImGui::IsItemActive()) {
			up = glm::rotate(glm::angleAxis(speed, right), up);
		}
		ImGui::Button("Down");
		if(ImGui::IsItemActive()) {
			up = glm::rotate(glm::angleAxis(-speed, right), up);
		}
		ImGui::Button("Clockwise");
		if(ImGui::IsItemActive()) {
			up = glm::rotate(glm::angleAxis(speed, front), up);
			right = glm::rotate(glm::angleAxis(speed, front), right);
		}
		ImGui::Button("Counter");
		if(ImGui::IsItemActive()) {
			up = glm::rotate(glm::angleAxis(-speed, front), up);
			right = glm::rotate(glm::angleAxis(-speed, front), right);
		}
		ImGui::End();

        right = glm::normalize(right);
		up = glm::normalize(up);
		front = glm::cross(up, right);
    }

    // pbr material
    static ImVec4 albedo;
	static float metallic;
	static float roughness;
	static float ao;
	static void material_control() {
		ImGui::Begin("material");
		ImGui::ColorPicker3("albedo", (float*)&albedo);
		ImGui::SliderFloat("metallic", &metallic, 0.0f, 1.0f);
		ImGui::SliderFloat("roughness", &roughness, 0.0f, 1.0f);
		ImGui::SliderFloat("ao", &ao, 0.0f, 1.0f);
		ImGui::End();
	}

	static float height_map_scale;
	static void height_map_control() {
		ImGui::Begin("height map");
		ImGui::SliderFloat("scale", &height_map_scale, 0.0f, 0.2f);
		ImGui::End();
	}
};

glm::vec3 Ctrl::model_euler = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 Ctrl::eye   = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 Ctrl::up    = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 Ctrl::right = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 Ctrl::front = glm::vec3(0.0f, 0.0f, -1.0f);

ImVec4 Ctrl::albedo   = ImVec4(0.5f, 0.5f, 0.5f, 0.5f);
float Ctrl::metallic  = 0.5f;
float Ctrl::roughness = 0.5f;
float Ctrl::ao        = 1.0f;

float Ctrl::height_map_scale = 0.0f;