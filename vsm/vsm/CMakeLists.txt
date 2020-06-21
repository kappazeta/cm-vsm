# ESA S2 product converter for cloud mask labeling and processing
#
# Copyright 2020 KappaZeta Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

vsm_set_source_files(${CMAKE_CURRENT_SOURCE_DIR} VSM)

add_executable(cvat_vsm ${VSM_SRC} ${VSM_INC})
target_link_libraries(cvat_vsm libvsm openjp2 png expat stdc++fs GraphicsMagick GraphicsMagick++)
set_target_properties(cvat_vsm PROPERTIES CXX_STANDARD 17)
