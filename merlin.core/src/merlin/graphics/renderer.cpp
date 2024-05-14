#include "glpch.h"
#include "merlin/core/core.h"
#include "renderer.h"



namespace Merlin {

	Renderer::Renderer() : currentTransform(glm::mat4(1.0)) {
		m_materialLibrary = MaterialLibrary::instance();
		m_shaderLibrary = ShaderLibrary::instance();
	}

	Renderer::~Renderer() {}


	void Renderer::initialize() {
		enableMultisampling();
		enableDepthTest();
		enableCubeMap();

		m_defaultEnvironment = createShared<Environment>("defaultEnvironment", 16);
	}

	void Renderer::pushMatrix() {
		matrixStack.push(currentTransform);
	}

	void Renderer::popMatrix() {
		if (!matrixStack.empty()) {
			currentTransform = matrixStack.top();
			matrixStack.pop();
		}
	}

	void Renderer::resetMatrix() {
		while(!matrixStack.empty()) {
			matrixStack.pop();
		}
		currentTransform = glm::mat4(1);
	}


	void Renderer::renderScene(const Scene& scene, const Camera& camera) {
		if (scene.hasEnvironment())m_currentEnvironment = scene.getEnvironment();
		for (const auto& node : scene.nodes()) {
			if(!node->isHidden()) render(node, camera);
		}
	}

	void Renderer::renderModel(const Model& mdl, const Camera& camera) {
		for (const auto& mesh : mdl.meshes()) {
			renderMesh(*mesh, camera);
		}
	}

	void Renderer::renderMesh(const Mesh& mesh, const Camera& camera) {

		Material_Ptr mat = mesh.getMaterial();
		Shader_Ptr shader = mesh.getShader();
		
		if (mesh.hasShader())
			shader = mesh.getShader();
		else
			shader = m_shaderLibrary->get(mesh.getShaderName());

		if (mesh.hasMaterial())
			mat = mesh.getMaterial();
		else 
			mat = m_materialLibrary->get(mesh.getMaterialName());
		

		shader->use();
		TextureBase::resetTextureUnit();
		mat->attach(*shader);
		
		if(m_currentEnvironment != nullptr)
			m_currentEnvironment->attach(*shader);
		else m_defaultEnvironment->attach(*shader);

		shader->setMat4("model", currentTransform); //sync model matrix with GPU
		shader->setMat4("view", camera.getViewMatrix()); //sync model matrix with GPU
		shader->setMat4("projection", camera.getProjectionMatrix()); //sync model matrix with GPU
		shader->setInt("numLights", m_activeLights.size());

		for (int i = 0; i < m_activeLights.size();  i++) {
			m_activeLights[i]->attach(i, *shader);
		}

		mesh.draw();
	}
	/*
	void Renderer::renderParticleSystem(const ParticleSystem& ps, const Camera& camera) {
		if (ps.getDisplayMode() == ParticleSystemDisplayMode::POINT_SPRITE) {
			const Shader* shader;
			//glPointSize(10);
			glEnable(GL_PROGRAM_POINT_SIZE);
			if (ps.getMesh()->hasShader())
				shader = &ps.getMesh()->getShader();
			else
				shader = &m_shaderLibrary.get(ps.getMesh()->getShaderName());

			shader->use();
			shader->setMat4("model", currentTransform); //sync model matrix with GPU
			shader->setMat4("view", camera.getViewMatrix()); //sync model matrix with GPU
			shader->setMat4("projection", camera.getProjectionMatrix()); //sync model matrix with GPU
			shader->setVec2("WindowSize", glm::vec2(camera.width(), camera.height())); //sync model matrix with GPU
			ps.draw(*shader);

			glDisable(GL_PROGRAM_POINT_SIZE);
		}
		else if (ps.getDisplayMode() == ParticleSystemDisplayMode::POINT_SPRITE_SHADED) {
			const Shader* shader;
			//glPointSize(10);
			glEnable(GL_PROGRAM_POINT_SIZE);
			glEnable(0x8861);//WTF
			//glDisable(GL_DEPTH_TEST);

			if (ps.getMesh()->hasShader())
				shader = &ps.getMesh()->getShader();
			else
				shader = &m_shaderLibrary.get(ps.getMesh()->getShaderName());


			shader->use();
			shader->setMat4("model", currentTransform); //sync model matrix with GPU
			shader->setMat4("view", camera.getViewMatrix()); //sync model matrix with GPU
			shader->setMat4("projection", camera.getProjectionMatrix()); //sync model matrix with GPU
			shader->setVec3("viewPos", camera.getPosition()); //sync model matrix with GPU
			shader->setVec2("WindowSize", glm::vec2(camera.width(), camera.height())); //sync model matrix with GPU
			ps.draw(*shader);

			//glEnable(GL_DEPTH_TEST);
			glDisable(GL_PROGRAM_POINT_SIZE);
			glDisable(0x8861);//WTF
		}
		else if (ps.getDisplayMode() == ParticleSystemDisplayMode::MESH) {
			const Shader* shader;
			Shared<MaterialBase> mat;

			if (ps.getMesh()->hasShader())
				shader = &ps.getMesh()->getShader();
			else
				shader = &m_shaderLibrary.get(ps.getMesh()->getShaderName());


			if (ps.getMesh()->hasMaterial())
				mat = ps.getMesh()->getMaterial();
			else {

				mat = m_materialLibrary.get(ps.getMesh()->getMaterialName());
			}


			shader->use();
			if (ps.getMesh()->hasShader())
				shader = ps.getMesh()->getShader();
			else
				shader = m_shaderLibrary.get(mesh.getShaderName());

			if (mesh.hasMaterial())
				mat = mesh.getMaterial();
			else
				mat = m_materialLibrary.get(mesh.getMaterialName());


			shader->setMat4("model", currentTransform); //sync model matrix with GPU
			shader->setMat4("view", camera.getViewMatrix()); //sync model matrix with GPU
			shader->setMat4("projection", camera.getProjectionMatrix()); //sync model matrix with GPU

			if (shader->supportTexture()) {
				Texture2D* tex = &mat->getTexture(TextureType::ALBEDO);

				//WARNING This should be done once...
				tex->setUnit(1); //Skybox is 0...
				tex->syncTextureUnit(*shader, (tex->typeToString()) + "0");


				tex->bind();
				shader->setInt("hasColorTex", !tex->isDefault());
			}
			ps.draw(*shader);
		}
	}
	*/
	void Renderer::renderTransformObject(const TransformObject& obj, const Camera& camera) {
		//TODO Render axis
		render(obj.getXAxisMesh(), camera);
		render(obj.getYAxisMesh(), camera);
		render(obj.getZAxisMesh(), camera);
	}

	void Renderer::render(const Shared<RenderableObject>& object, const Camera& camera) {
		pushMatrix();
		currentTransform *= object->transform();

		if (object->isWireFrame()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//The object is a mesh

		if (const auto light = std::static_pointer_cast<Light>(object)) {
			light->applyRenderTransform(currentTransform);
			m_activeLights.push_back(light);
		}
		else if (const auto mesh = std::dynamic_pointer_cast<Mesh>(object)) {
			renderMesh(*mesh, camera);
		}//The object is a model
		else if (const auto model = std::dynamic_pointer_cast<Model>(object)) {
			renderModel(*model, camera);
		}//The object is a scene
		else if (const auto scene = std::dynamic_pointer_cast<Scene>(object)) {
			renderScene(*scene, camera); //Propagate to childrens
		}//The object is a scene node
		else if (const auto ps = std::dynamic_pointer_cast<TransformObject>(object)) {
			renderTransformObject(*ps, camera); //Propagate to childrens
		}
		for (auto node : object->children()) {
			render(node, camera);//Propagate to childrens
		}

		popMatrix();
	}

	Shared<Shader> Renderer::getShader(std::string n) {
		return m_shaderLibrary->get(n);
	}

	void Renderer::loadShader(const std::string& name, const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const std::string& geomShaderPath) {
		Shared<Shader> shader = Shader::create(name, vertexShaderPath, fragmentShaderPath, geomShaderPath);
		m_shaderLibrary->add(shader);
	}

	void Renderer::addMaterial(Shared<MaterialBase> material) {
		m_materialLibrary->add(material);
	}

	void Renderer::addShader(Shared<Shader> shader) {
		if (!shader->isCompiled()) Console::error("Renderer") << "Shader is not compiled. Compile the shader before adding them to the ShaderLibrary" << Console::endl;
		m_shaderLibrary->add(shader);
	}

	void Renderer::clear() {
		RendererBase::clear();
		resetMatrix();
		m_activeLights.clear();
	}




}