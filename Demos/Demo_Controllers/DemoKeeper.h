/*!
	@file
	@author		Albert Semenov
	@date		08/2008
*/
#ifndef DEMO_KEEPER_H_
#define DEMO_KEEPER_H_

#include "Base/BaseDemoManager.h"
#include "State.h"

namespace demo
{

	class DemoKeeper :
		public base::BaseDemoManager
	{
	public:
		DemoKeeper();

		void createScene() override;
		void destroyScene() override;

	private:
		void notifyButtonPress(ControllerType _type, bool _complite);
		void setupResources() override;

	private:
		State* mMenu1;
		State* mMenu2;
		State* mMenu3;
		State* mMenu4;
		ControllerType mType;
		bool mIsAnim;
	};

} // namespace demo

#endif // DEMO_KEEPER_H_
