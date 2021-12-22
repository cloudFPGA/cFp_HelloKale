/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

/*****************************************************************************
 * @file       LinkDef.h
 * @brief      A header file defining a logical hierarchy of groups/subroups
 *             for the documentation of the current project.
 * @author     did, fab
 * @details    The structure is as follows:
 *             [+] cFp_HelloKale
 *              +-- [+] tcp_app_flash
 *              |    +-- tcp_app_flast_test  
 *              +-- [+] tcp_shell_if
 *              |    +-- tcp_shell_if_test
 *              +-- [+] udp_app_flash
 *              |    +-- udp_app_flash_test
 *              +-- [+] udp_shell_if
 *                   +-- udp_shell_if_test
 *
 * @note: Every cFp_<Project> must be part of the 'cFp' module.
 *****************************************************************************/


/*****************************************************************************
 *
 *  cFp_HelloKale : This is a 'Hello World' project targetting the SHELL of 
 *   type 'Kale' which is a shell with minimalist support for accessing the
 *   hardware components of the FPGA card.
 *   The ROLE of the project implements a set of TCP- and UDP-oriented tests
 *   and functions to experiment with network sockets.
 *
 *****************************************************************************/

/** \defgroup cFp_HelloKale cFp_HelloKale
 *  @ingroup cFp
 * 
 *  \brief This is the root of the current 'cFp_HelloKale' project.
 */

/*****************************************************************************
 *
 *  cFp_HelloKale / Submodules
 *
 *****************************************************************************/

/** \defgroup tcp_app_flash tcp_app_flash 
 *  @ingroup cFp_HelloKale
 * 
 *  \brief  This module implements a set of TCP-oriented tests and functions.
 *
 */

/** \defgroup tcp_shell_if tcp_shell_if 
 *  @ingroup cFp_HelloKale
 * 
 *  \brief  This module handles the control flow interface between the SHELL and the ROLE.
 */

/** \defgroup udp_app_flash udp_app_flash 
 *  @ingroup cFp_HelloKale
 * 
 *  \brief  This module implements a set of UDP-oriented tests and functions.
 *
 */

/** \defgroup udp_shell_if udp_shell_if 
 *  @ingroup cFp_HelloKale
 * 
 *  \brief  This module handles the control flow interface between the SHELL and the ROLE.
 */

