/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OK(call) assert(!rohr_error_check(call))
#define BAD(call) assert(rohr_error_check(call))
static InputControllerId create(ControllerDescription description) {
    InputControllerIdResult result=rohr_controller_create(description); OK(result); return result.result.value;
}
static InputActionId action(InputControllerId controller, const char *name) {
    InputActionIdResult result=rohr_input_action_by_name_get(controller,name); OK(result); return result.result.value;
}
static ControllerDescription recipe(ControllerActionDescription *actions) {
    actions[0]=(ControllerActionDescription){.name="toggle",.type=INPUT_ACTION_BUTTON,
        .button_mode=INPUT_BUTTON_PERSISTENT,.button_initial_state=true};
    actions[0].bindings_default.count=1;
    actions[0].bindings_default.values[0]=rohr_input_binding_scancode_create(SDL_SCANCODE_F5);
    actions[1]=(ControllerActionDescription){.name="move",.type=INPUT_ACTION_AXIS_1D};
    return (ControllerDescription){.controller={.name="player",.enabled=true},.actions=actions,.action_count=2};
}
static void creation_and_copy(void) {
    ControllerActionDescription actions[2]; ControllerDescription description=recipe(actions);
    unsigned calls=0;
    InputControllerIdResult result=controller_create((&description)[calls++]); OK(result); assert(calls==1);
    InputControllerId first=result.result.value; InputActionId toggle=action(first,"toggle");
    InputControllerId duplicate=create(description);
    assert(!strcmp(rohr_input_controller_name_get(duplicate).result.value.value,"player_1"));
    OK(rohr_input_controller_destroy(duplicate));
    strcpy(description.controller.name,"second"); InputControllerId second=create(description);
    OK(rohr_input_action_button_state_set(toggle,false));
    assert(rohr_input_action_button_down_check(action(second,"toggle")));
    memset(actions,0,sizeof(actions)); memset(&description,0,sizeof(description));
    InputActionDescriptionResult saved=rohr_input_action_description_get(toggle); OK(saved);
    assert(saved.result.value.bindings_default.count==1);
    assert(saved.result.value.bindings_default.values[0].input.key==SDL_SCANCODE_F5);
    OK(rohr_input_controller_destroy(first)); BAD(rohr_input_action_description_get(toggle));
    assert(rohr_input_action_button_down_check(action(second,"toggle")));
    OK(rohr_input_controller_destroy(second));
    InputControllerDescription native={.name="empty",.enabled=false}; calls=0;
    result=rohr_controller_create((&native)[calls++]); OK(result); assert(calls==1);
    assert(!rohr_input_controller_enabled_check(result.result.value));
    OK(rohr_input_controller_destroy(result.result.value));
    result=rohr_controller_create((InputControllerDescription){.name="temporary",.enabled=true}); OK(result);
    OK(rohr_input_controller_destroy(result.result.value));
    ControllerDescription defaults=rohr_controller_description_default_get();
    assert(defaults.controller.enabled && !defaults.action_count && !defaults.actions);
    BAD(rohr_controller_create(defaults));
}
static void failed_attempts(void) {
    ControllerActionDescription actions[2]; ControllerDescription description=recipe(actions);
    InputControllerId survivor=create(description); strcpy(description.controller.name,"attempt");
    InputActionId toggle=action(survivor,"toggle");
    for(unsigned failure=0;failure<8;++failure) {
        ControllerActionDescription bad[2]; ControllerDescription invalid=recipe(bad);
        strcpy(invalid.controller.name,"attempt");
        switch(failure) {
        case 0: invalid.actions=NULL; break;
        case 1: invalid.action_count=ROHR_INPUT_ACTION_LIMIT+1; break;
        case 2: strcpy(bad[1].name,bad[0].name); break;
        case 3: bad[1].type=(InputActionType)999; break;
        case 4: bad[1].bindings_default.count=ROHR_INPUT_BINDING_LIMIT+1; break;
        case 5: memset(bad[1].name,'x',sizeof(bad[1].name)); break;
        case 6: bad[1].type=INPUT_ACTION_BUTTON; bad[1].button_mode=(InputButtonMode)999; break;
        case 7: bad[1].bindings_default.count=1; bad[1].bindings_default.values[0].source=(InputBindingSource)999; break;
        }
        BAD(rohr_controller_create(invalid));
        BAD(rohr_input_controller_by_name_get("attempt"));
        assert(rohr_input_action_button_down_check(toggle));
        InputControllerId fresh=create(description); OK(rohr_input_controller_destroy(fresh));
    }
    OK(rohr_input_controller_destroy(survivor));
}
static void capacities(void) {
    InputControllerIdResult filler=rohr_controller_create((InputControllerDescription){.name="filler",.enabled=true}); OK(filler);
    for(unsigned i=0;i<ROHR_INPUT_ACTION_LIMIT-1;++i) {
        InputActionDescription action={.controller=filler.result.value,.type=INPUT_ACTION_BUTTON};
        snprintf(action.name,sizeof(action.name),"action_%u",i); OK(rohr_input_action_create(action));
    }
    ControllerActionDescription actions[2]; ControllerDescription description=recipe(actions);
    InputControllerIdResult failed=rohr_controller_create(description); BAD(failed);
    assert(failed.result.error==ERROR_ENGINE_INPUT_CAPACITY_EXCEEDED);
    BAD(rohr_input_controller_by_name_get("player"));
    /* The first action consumed the last slot before the second failed. Its
     * slot must be available again and all pre-existing actions must survive. */
    OK(rohr_input_action_create((InputActionDescription){.controller=filler.result.value,.name="last"}));
    for(unsigned i=0;i<ROHR_INPUT_ACTION_LIMIT-1;++i) {
        char name[ROHR_INPUT_NAME_MAX]; snprintf(name,sizeof(name),"action_%u",i);
        OK(rohr_input_action_by_name_get(filler.result.value,name));
    }
    OK(rohr_input_controller_destroy(filler.result.value));
    InputControllerId controllers[ROHR_INPUT_CONTROLLER_LIMIT];
    for(unsigned i=0;i<ROHR_INPUT_CONTROLLER_LIMIT;++i) {
        InputControllerDescription native={.enabled=true}; snprintf(native.name,sizeof(native.name),"c_%u",i);
        InputControllerIdResult result=rohr_controller_create(native); OK(result); controllers[i]=result.result.value;
    }
    BAD(rohr_controller_create(description));
    for(unsigned i=0;i<ROHR_INPUT_CONTROLLER_LIMIT;++i) OK(rohr_input_controller_destroy(controllers[i]));
}
int main(void) {
    OK(rohr_engine_start());
    creation_and_copy(); failed_attempts(); capacities();
    ControllerActionDescription actions[2]; const ControllerDescription description=recipe(actions);
    for(unsigned i=0;i<1000;++i) {
        InputControllerId owner=create(description); InputActionId child=action(owner,"toggle");
        OK(rohr_input_controller_destroy(owner));
        BAD(rohr_input_controller_description_get(owner)); BAD(rohr_input_action_description_get(child));
    }
    rohr_engine_stop(); return 0;
}
