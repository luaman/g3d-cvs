#include "G3D/G3DAll.h"

void function() {
	//printf("Function");
}

class Base : public ReferenceCountedObject {
public:
	void method() {
		//printf("Method");
	}

	virtual void method2() {
		//printf("Method 2");
	}
};

class Class : public Base {
public:
	virtual void method2() {
		//printf("Method 2 Override");
	}
};

void testCallback() {    
	printf("GuiControl::Callback ");

	Base base;
	Class object;
	ReferenceCountedPointer<Base> basePtr = new Base();
	ReferenceCountedPointer<Class> ptr = new Class();

	object.method();
	ptr->method();
	function();

	GuiControl::Callback funcCall(&function);

	// Test methods
	GuiControl::Callback baseCall(&base, &Base::method);
	GuiControl::Callback baseRefCall(basePtr, &Base::method);

	// Test inherited methods
	GuiControl::Callback objCall((Base*)&object, &Base::method);
	GuiControl::Callback obj2Call(&object, &Class::method2);
	GuiControl::Callback objRefCall(ReferenceCountedPointer<Base>(ptr), &Base::method);
	printf("passed\n");
}


