/*!
	@file
	@author		Albert Semenov
	@date		11/2007
	@module
*/
#include "MyGUI_SkinManager.h"
#include "MyGUI_WidgetSkinInfo.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_SubWidgetManager.h"
#include "MyGUI_Gui.h"

namespace MyGUI
{

	const std::string XML_TYPE("Skin");

	INSTANCE_IMPLEMENT(SkinManager);

	void SkinManager::initialise()
	{
		MYGUI_ASSERT(false == mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		ResourceManager::getInstance().registerLoadXmlDelegate(XML_TYPE) = newDelegate(this, &SkinManager::_load);

		createDefault();

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void SkinManager::shutdown()
	{
		if (false == mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		ResourceManager::getInstance().unregisterLoadXmlDelegate(XML_TYPE);

		for (MapWidgetSkinInfoPtr::iterator iter=mSkins.begin(); iter!=mSkins.end(); ++iter) {
			WidgetSkinInfoPtr info = iter->second;
			info->clear();
			delete info;
		}
		mSkins.clear();

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

	WidgetSkinInfo * SkinManager::getSkin(const Ogre::String & _name)
	{
		MapWidgetSkinInfoPtr::iterator iter = mSkins.find(_name);
		// ���� �� �����, �� ������ ��������� ����
		if (iter == mSkins.end()) {
			MYGUI_LOG(Warning, "Skin '" << _name << "' not found, set Default");
			return mSkins["Default"];
		}
		return iter->second;
	}

	//	��� ������� �������� �����
	WidgetSkinInfo * SkinManager::create(const Ogre::String & _name)
	{
		WidgetSkinInfo * skin = new WidgetSkinInfo();
		if (mSkins.find(_name) != mSkins.end()){
			MYGUI_LOG(Warning, "Skin with name '" + _name + "' already exist");
			mSkins[_name]->clear();
			delete mSkins[_name];
		}
		mSkins[_name] = skin;
		return skin;
	}

	bool SkinManager::load(const std::string & _file, const std::string & _group)
	{
		return ResourceManager::getInstance()._loadImplement(_file, _group, true, XML_TYPE, INSTANCE_TYPE_NAME);
	}

	void SkinManager::_load(xml::xmlNodePtr _node, const std::string & _file)
	{
		// ��������������� ����� ��� �������� ���������
		SubWidgetBinding bind;

		// ����� ����� � ��������, �������� ���� �� �������
		xml::xmlNodeIterator skin = _node->getNodeIterator();
		while (skin.nextNode(XML_TYPE)) {

			// ������ �������� �����
			Ogre::String name, texture, tmp;
			IntSize size;
			skin->findAttribute("name", name);
			skin->findAttribute("texture", texture);
			if (skin->findAttribute("size", tmp)) size = IntSize::parse(tmp);

			// ������� ����
			WidgetSkinInfo * widget_info = create(name);
			widget_info->setInfo(size, texture);
			IntSize materialSize = getTextureSize(texture);

			// ��������� �����
			if (skin->findAttribute("mask", tmp)) {
				if (false == widget_info->loadMask(tmp)) {
					MYGUI_LOG(Error, "Skin: " << _file << ", mask not load '" << tmp << "'");
				}
			}

			// ����� ����� � ��������, ���� � ��� �������
			xml::xmlNodeIterator basis = skin->getNodeIterator();
			while (basis.nextNode()) {

				if (basis->getName() == "Property") {
					// ��������� ��������
					std::string key, value;
					if (false == basis->findAttribute("key", key)) continue;
					if (false == basis->findAttribute("value", value)) continue;
					// ��������� ��������
					widget_info->addProperty(key, value);

				}
				else if (basis->getName() == "Child") {
					ChildSkinInfo child(
						basis->findAttribute("type"),
						basis->findAttribute("skin"),
						basis->findAttribute("name"),
						IntCoord::parse(basis->findAttribute("offset")),
						Align::parse(basis->findAttribute("align")),
						basis->findAttribute("layer")
						);

					xml::xmlNodeIterator child_params = basis->getNodeIterator();
					while (child_params.nextNode("Property"))
						child.addParam(child_params->findAttribute("key"), child_params->findAttribute("value"));

					widget_info->addChild(child);
					//continue;

				}
				else if (basis->getName() == "BasisSkin") {
					// ������ ��������
					Ogre::String basisSkinType, tmp;
					IntCoord offset;
					Align align = Align::Default;
					basis->findAttribute("type", basisSkinType);
					if (basis->findAttribute("offset", tmp)) offset = IntCoord::parse(tmp);
					if (basis->findAttribute("align", tmp)) align = Align::parse(tmp);

					bind.create(offset, align, basisSkinType);

					// ����� ����� � ��������, ���� �� ��������
					xml::xmlNodeIterator state = basis->getNodeIterator();
					while (state.nextNode()) {

						if (state->getName() == "State") {
							// ������ �������� ������
							Ogre::String basisStateName;
							state->findAttribute("name", basisStateName);

							// ������������ ���� � ������
							StateInfo * data = SubWidgetManager::getInstance().getStateData(basisSkinType, state.currentNode(), skin.currentNode());

							// ��������� ���� � ������
							bind.add(basisStateName, data);
						}
						else if (state->getName() == "Property") {
							// ��������� ��������
							std::string key, value;
							if (false == state->findAttribute("key", key)) continue;
							if (false == state->findAttribute("value", value)) continue;
							// ��������� ��������
							bind.addProperty(key, value);
						}

					};

					// ������ �� ������ ��������� � ����
					widget_info->addInfo(bind);
				}

			};
		};
	}	

	IntSize SkinManager::getTextureSize(const std::string & _texture)
	{
		// ��������� ��������
		static std::string old_texture;
		static IntSize old_size;

		if (old_texture == _texture) return old_size;
		old_texture = _texture;
		old_size.clear();

		if (_texture.empty()) return old_size;

		Ogre::TextureManager & manager = Ogre::TextureManager::getSingleton();
		if (false == manager.resourceExists(_texture)) {

			if (!helper::isFileExist(_texture, Gui::getInstance().getResourceGroup())) {
				MYGUI_LOG(Error, "Texture '" + _texture + "' not found");
				return old_size;
			}
			else {
				manager.load(
					_texture,
					Gui::getInstance().getResourceGroup(),
					Ogre::TEX_TYPE_2D,
					0);
			}
		}

		Ogre::TexturePtr tex = (Ogre::TexturePtr)manager.getByName(_texture);
		if (tex.isNull()) {
			MYGUI_LOG(Error, "Texture '" + _texture + "' not found");
			return old_size;
		}
		tex->load();

		old_size.set((int)tex->getWidth(), (int)tex->getHeight());

#if MYGUI_DEBUG_MODE == 1
		if (isPowerOfTwo(old_size) == false) {
			MYGUI_LOG(Warning, "Texture '" + _texture + "' have non power ow two size");
		}
#endif

		return old_size;
	}

	FloatRect SkinManager::convertTextureCoord(const FloatRect & _source, const IntSize & _textureSize)
	{
		if (!_textureSize.width || !_textureSize.height) return FloatRect();
		return FloatRect(
			_source.left / _textureSize.width,
			_source.top / _textureSize.height,
			(_source.left + _source.right) / _textureSize.width,
			(_source.top + _source.bottom) / _textureSize.height);
	}

	void SkinManager::createDefault()
	{
		// ������� ��������� ����
		WidgetSkinInfo * widget_info = create("Default");
		widget_info->setInfo(IntSize(0, 0), "");
	}

	bool SkinManager::isPowerOfTwo(IntSize _size)
	{
		int count = 0;
		while (_size.width > 0) {
			count += _size.width & 1;
			_size.width >>= 1;
		};
		if (count != 1) return false;
		count = 0;
		while (_size.height > 0) {
			count += _size.height & 1;
			_size.height >>= 1;
		};
		if (count != 1) return false;
		return true;
	}

} // namespace MyGUI
