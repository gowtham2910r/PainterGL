class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;
    float pitch;
    float fov;

    Camera(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch, float startFov)
        : position(startPosition), up(startUp), yaw(startYaw), pitch(startPitch), fov(startFov) {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float aspectRatio) const {
        return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
    }

    void processKeyboard(char key, float deltaTime) {
        float velocity = 3.0f * deltaTime;
        if (key == 'w')
            position += front * velocity;
        if (key == 's')
            position -= front * velocity;
        if (key == 'a')
            position -= glm::normalize(glm::cross(front, up)) * velocity;
        if (key == 'd')
            position += glm::normalize(glm::cross(front, up)) * velocity;
	if (key == 'q')
            position += up * velocity;
    	if (key == 'e') 
            position -= up * velocity;
    }

    void processMouseMovement(float xOffset, float yOffset) {
        float sensitivity = 0.1f;
        xOffset *= sensitivity;
        yOffset *= sensitivity;

        yaw += xOffset;
        pitch += yOffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        updateCameraVectors();
    }

private:
    void updateCameraVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);
    }
};