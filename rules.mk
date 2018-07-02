$(BUILD_DIR)/%/.dummy:
	mkdir -p $(@D)
	touch $@

$(BUILD_DIR)/.dummy:
	mkdir -p $(@D)
	touch $@

$(BUILD_DIR)/%.cpp.o: %.cpp | $$(@D)/.dummy
	$(CXX) -o $@ -c -MMD -MP $(INCLUDES) $(CXXFLAGS) $<

clean:
	rm -rf $(BUILD_DIR)

# print_vars:
# 	@echo deps: $(DEPS)
# 	@echo CXX: $(CXX)
