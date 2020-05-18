/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * Yul code and data object container.
 */

#include <libyul/Object.h>

#include <libyul/AsmPrinter.h>
#include <libyul/Exceptions.h>

#include <libsolutil/Visitor.h>
#include <libsolutil/CommonData.h>

#include <boost/algorithm/string/replace.hpp>

using namespace std;
using namespace solidity;
using namespace solidity::yul;
using namespace solidity::util;

namespace
{

string indent(std::string const& _input)
{
	if (_input.empty())
		return _input;
	return boost::replace_all_copy("    " + _input, "\n", "\n    ");
}

}

string Data::toString(Dialect const*) const
{
	return "data \"" + name.str() + "\" hex\"" + util::toHex(data) + "\"";
}

string Object::toString(Dialect const* _dialect) const
{
	yulAssert(code, "No code");
	string inner = "code " + (_dialect ? AsmPrinter{*_dialect} : AsmPrinter{})(*code);

	for (auto const& obj: subObjects)
		inner += "\n" + obj->toString(_dialect);

	return "object \"" + name.str() + "\" {\n" + indent(inner) + "\n}";
}

set<YulString> Object::dataNames() const
{
	set<YulString> names;
	names.insert(name);
	for (shared_ptr<ObjectNode> const& subObjectNode: subObjects)
	{
		names.insert(subObjectNode->name);
		if (auto const* subObject = dynamic_cast<Object const*>(subObjectNode.get()))
			for (YulString const& subSubObj: subObject->dataNames())
				names.insert(YulString{subObject->name.str() + "." + subSubObj.str()});
	}

	names.erase(YulString{});
	return names;
}

map<YulString, shared_ptr<ObjectNode>> Object::subObjectsByDataName() const
{
	map<YulString, shared_ptr<ObjectNode>> objectsByDataName;
	for (shared_ptr<ObjectNode> const& subObjectNode: subObjects)
		if (auto const* subObject = dynamic_cast<Object const*>(subObjectNode.get()))
		{
			set<YulString> subObjDataNames = subObject->dataNames();
			for (YulString const& subObjDataName: subObjDataNames)
				if (subObjDataName == subObject->name)
					objectsByDataName[subObject->name] = subObjectNode;
				else
				{
					map<YulString, shared_ptr<ObjectNode>> subSubObjects = subObject->subObjectsByDataName();
					if (subSubObjects.count(subObjDataName))
						objectsByDataName[YulString{subObject->name.str() + "." + subObjDataName.str()}] =
							subObject->subObjectsByDataName().at(subObjDataName);
				}
		}

	return objectsByDataName;
}

void Object::addNamedSubObject(YulString _name, shared_ptr<ObjectNode> _subObject)
{
	if (subIndexByName.count(_name) == 0)
	{
		subIndexByName[_name] = subObjects.size();
		subObjects.emplace_back(std::move(_subObject));
	}
}
