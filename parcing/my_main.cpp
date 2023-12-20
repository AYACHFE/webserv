/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   my_main.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aachfenn <aachfenn@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/12/15 11:26:32 by aachfenn          #+#    #+#             */
/*   Updated: 2023/12/20 11:55:05 by aachfenn         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parceConfFile.hpp"

// int main() {

// 	try
// 	{
// 		parceConfFile parce;

// 		parce.read_conf_file();
// 		parce.fill_data();
// 	}
// 	catch (std::exception &e) {
// 		cout << e.what() << endl;
// 	}
// 	catch (...) {
// 		cout << "Error" << endl;
// 	}
// }

void parce_conf_file(parceConfFile &parce) {

	try
	{
		// parceConfFile parce;

		parce.read_conf_file();
		parce.fill_data();
		// return parce;
	}
	catch (std::exception &e) {
		cout << e.what() << endl;
	}
	catch (...) {
		cout << "Error" << endl;
	}
}