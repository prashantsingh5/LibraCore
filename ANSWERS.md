// Name: Prashant Singh
// Roll No: MT26076

# Assignment-2 answers

## Q1 - Journal

we have created a "Joural class" that is derived from "Resources.

What is added that is not before:
- ISSN - International Standard Serial Number
- number of issues per year
- subscription length in years

The subscription length is set to 1 year by default default. If a a value less than 1 is provided throws `std::invalid_argument`.

for journals, `unitPrice` represents the annual subscription price for single copy. So the purchase cost is calculated by:

`unitPrice * copies * subscriptionYears`

A seperate `journal` value is added to `ResourceCategory` so that jpournals has their own budget quota and can be showed seperately in budget report.

`Journal::printDetails()` is overridden to print the ISSN (International Standard Serial Number), issues per year, and subscription years.

Tests and the demos are also extended to cover the new Journal functionalities.

## Q2 - EBook

Added an `EBook` class that is derived from `ElectronicResources`.

This class stores:
- authors
- ISBN
- file format allowed ('PDF',EPUB', 'HTML')
- whether the ebook DRM protected or not.

This inherits its pricing beghaviour from `ElectronicCategory` without obverriding `costfor`. So, its cost is calculated using only the existing electronic-resource pricing:

`platformFee + unitPrice * numberOfSeats`

A seperate `EBook` value is added to `ResourceCategory` so ebooks can have their own quota's seperate from other electronic resources.

### Design answer
I have already added a comment regarding this in `EBook.h` jsut adding here for easier access such that i can read without looking too much.

Book and Ebook both has some common information ie, authors, ISBN that we are adding in both Book nad EBook classes, This is duplication. So to resolve this we could add a parent calss that contain this information once, then they can just inherit the parent class.

Tests and the demo were extended to cover the new EBook functionality.

## Q3 - AudioBook and Thesis

Added an `AudioBook` class that is being derived from `ElectronicResorces` so that it only need to store narrator and duration in minutes.

`ElectronicResource` is chosen as the base class because audiobook is a digital resource. due to which we can reuse the existing electronic access, licensing, platform-free and pricing behaviour without duplicating it.

we also added a `Thesis` class that is directly derived from `Resource`. so it only need to store university, degree and supervisor.

`Resource` is chosen as base class because it is also a library resource but it dont necessaryu have all the properties of `Book`, like  ISBN, edition or binding. A thesis class a default unit price of `0.00`, Since Thesis are generally free of cost, while still allowing to add a price is required.

Tests and the demo were extended to cover both classes and their inherited
behavior.

## Q4 - Hardcover Pricing

`Book::costFor()` is overriden so that only hardcover book cost 20% more than the listed unit price. but paperback pricing remains unchanged.

the surchange is being calculated using the integer minor-unit representation of `Money` instead of floating pount arithmetic, so it can avoid floating point rounding errors.

The demo and tests were updated to verify both paperback and hardcover pricing.

## Q5 - Bulk Pricing Rules

for print resources, bulk order of 10 or more recieve a 10% discount.
So, the discount is implementerd in relevant print-resources pricing logic and for `Book`, the hardcover is applied first and then 10% bulk discount is applied to the resulting total.

For `Jouranl`, the normal subscription cost is calculated using:

`unitPrice * copies * subscriptionYears`

and then the 10% bulk discount is applied when 10 or more copies are ordered on top of that.

and for general print resources using `Resource::costFor()`, the same 10% discount is applied when the quantity is atleast 10 or more.

and for `ElectronicResource`, the first 50 seats are at full price and only seats beyond 50 will be charged at half the normal price but the platform charge is still charged only once.

Since, both `EBook` and `AudioBook` inherit electronic pricing from
`ElectronicResource` they are automatically using same bulk pricing rule.

Tests and the demo were extended to verify the exact threshold and pricing
behavior.

## Q6 - Taxes

tax rates for print and electronic resources are configured with `TaxRates`, it stores seperate percentage for both print and electronic items.

Tax is being applied in `AcquisitionManager` rather than in individual resource
classes. So, `Resource::costFor()` and its override functions continue to calculate the prices before tax, It includes existing rules like hardcover and bulk discounts.

Resources are classified for taxes using `Resource::isDigital()`, this makes sures that `ElectronicResource`, `EBook`, and `AudioBook` all recieves electronic tax rate, whereas `Book`, `Journal`, and `Thesis` are receiving the print based tax rates.

every purchased record stores both pre and post tax costs. The existing `cost` field is considered as final post tax amt to preserve the compatibility with existing public interface.

`quote()`, `canPurchase()`, `purchase()`, and batch processing all are applying taxes. both Budget and quota checks are using post-tax cost, as required.

in report both pre and post tax values are displayed for indiviadual orders, and total pre-tax and total post-tax spending are also being showed.

Negative tax rates are rejected with `std::invalid_argument`.

## Q7 - Different-Title Quota

Now category quota can limit the number of different resource titles that may be purchased when needed means optionally.

So title limit is stored as an optional `maxTitles` value in `Quota`. So, if no title limit is supplied, it will behave as before and has no different-title restrictions.

`Budget` tracks the successfully purchased resources IDs in a set for each category. SO a resource category therefore counted only once, even if additional copies or seats of same resource are purchased later.

`AcquisitionManager` passes the resource's IDs to `Budget::check()` and `Budget::commit()` so that the budget can successfully distinguish bw an existing title and a new title.

New title is rejected if the title limit is reached for a category. But purchases of titles that are already counted in that category are still allowed, when provided the the unit, spend, and overall budget limits are satisfied.

the pyrchases that are rejected do not consume a title slot.

Now budget report includes a `Titles used/max` column along with unit and spending usage.

## Q8 - Cancellation

Now the approved orders can be cancelled too using the `acquisition manager.`

Cancelling an order does not erase the purchase record from the history.
A cancellation record is generated instead which contains the order number of the original purchase.

A cancellation gives back the exact amount charged against the budget after tax. Usage of category units, category spend, and budget spending is decreased accordingly.

The catalog holdings are decreased with the quantity of the cancelled order.

For proper functioning with the Q7 `different title quota` system, the budget tracks the active purchased quantity for each resource ID. An order can be cancelled but it doesn't release the title slot of the title as long as there are other active orders for the title. A title is removed from the title usage of a category if the active purchased quantity becomes zero.

The order can be cancelled only once. An order that is rejected, any
cancellation record, and unknown order number cannot be cancelled.

The `acquisition reports` differentiate cancellation record from regular purchase and rejected orders. The totals pre-tax and post-tax are net totals after refunds.

## Q9 - Department Budgets

the `AcquisitionManager` can now register seperate bugets for departments like Science and Physics.

eacch departament has its own `budget` object, its overall budget, category unit quotas, category spend quotas, also different-title quotas are being tracked seperately from other departments.

Now, `PurchaseRequest` has a field for department name. In purchases, affordability checks, and batches, the budget of the specified department is
used.

Requests without a department use the default budget that was used before.

Purchases have information on the department that was charged, and acquisition reports have information on the department for departmental requests.

Catalogs and holdings are still department-independent. So copies/ seats purchased from different departments add up to catalog holdings but maintain separate financial usage.

Cancelation also uses the department from the original request, so the refund goes back to the same department budget that paid for the order.

Departments that are unknown are rejected. Duplicate and empty department registrations are not permitted.

## Q10 - Year-End Rollover

The `Budget` class now includes a `nextYear()` method that creates a new budget for the following year.

The client program passes an annual allocation and a customizable carry-forward percentage from 0% to 100%.

The carried amount is determined by the following formula based on the remaining budget of the current budget:

`carried amount = current remaining budget * carry %`

The carried amount is added to the new year allocation and determines the new budget total.

The category quota definitions, which include unit, spend and different title quotas, are kept intact in the new-year budget.

The current year spending statistics are intentionally not carried over. The new budget is initialized with a zero total spending, category units used and titles purchased.

The old budget remains unchanged, thus, the next-year budget is a new object.

If the carried amount exceeds 100% or is less than 0%, or the new year allocation is negative, an exception `std::invalid_argument` is thrown.

Since the departmental budgets in Q9 are standard `Budget` objects, each department can generate its own next-year budget in the same manner.

## Q11 - All-or-Nothing Batch Processing

The method `AcquisitionManager::processBatch()` now supports an option for all-or-nothing mode.

The default mode of batch processing was maintained in case atomicity isn't required: requests will be processed separately and all successful purchases will be kept regardless of failure of the following request.

When atomic mode is enabled, the whole batch is validated with the use of temporary copies of the corresponding budgets. There are no real modifications of budgets and catalog holdings done at the validation step.

All requests that passed validation get added to the budget as part of simulation and all further requests within the same batch take into account
the effects of previous requests. Thus, atomic validation can identify failures caused by unit quotas, spend quotas, budget limits, and different
titles quotas combined together.

Budgets for departments are simulated separately, therefore, department-specific quotas introduced in Q9 are honored as well.

If some of the requests failed the validation process, all the requests in the atomic batch will be marked as failed and won't lead to any real purchases.

In case all the requests were successfully processed during simulation, the batch gets applied to the real budgets and catelog holdings.

The old single parameter `processBatch()` method is still present but forwards to the new non-atomic method.

## Q12 - Vendors

Resources are now able to contain many vendor offers, with each offer consisting of a vendor name and a unit price.

`Resource` class implements functionality for adding vendor offers, listing them and getting the cheapest vendor.

Purchases will now be made with the cheapest vendor offer. If there are no vendor offers registered for a resource, its original unit price will be used and the vendor name will not be filled out in the order record.

Vendor prices are used as replacement unit prices in order to maintain
pricing rules of every resource type.

For instance:

- hardcovers are still receiving Q4 20% surcharge;
- print resources are still getting Q5 bulk discounts;
- electronic resources still have platform fee and 50+ half-price seats;
- journal subscriptions are still multiplying their vendor annual price on their subscription length;
- Q6 tax is calculated based on pre-tax vendor-based price.

`costForAtPrice()` virtual function was introduced to allow different polymorphic resource types calculate their prices according to their rules, using a supplied vendor price.

`AcquisitionManager` is using the same cheapest vendor in quoting,
affordability, normal purchasing, batch process and Q11 atomic batch simulations.

The chosen vendor is saved in `PurchaseRecord` and displayed in the acquisition reports.

When cancelling an order, the information about the vendor remains the same as before while the refund is provided in full.

Invalid vendor names and prices less than zero throw's exceptions of `std::invalid_argument`.

## Q13 - Catalog Searches

More searches have been added to the catalog to operate on authors, ISBN/ISSN identifiers, and publication years.

Author search is not case-sensitive and allows partial names. It operates on both physical `Book` resources and `EBook` resources.

ISBN search is performed on both `Book` and `EBook` resources.

ISSN search is done on `Journal` resources.

Finally there is an identifier search which is used to search either the ISBN or ISSN values.

Publication-year search takes an inclusive lower and upper year bound and searches for resources which have their publication year between those bounds.

An exception `std::invalid_argument` is thrown when the lower year is greater than the upper one.

Searches return references to the resources in the catalog rather than creating new copies. This is because the catalog holds resources in `std::map` which keeps them ordered by resource-id.

Implementation of these searches makes use of the already existing `where()` query function in the catalog and only performs polymorphic type-checks when required.

## Q14 - Lending and Concurrent Electronic Sessions

Now the catalog stores both the total amount and the number of items lent/being used.

For print resources, the lending quantity stands for the number of physical items that are currently lent.

For electronic resources, the same thing means the number of current concurrent sessions that use the available number of licensed seats.

The catalog provides the following operations:

- get the number of lent or active units;
- get the number of available units at the moment;
- lend one or more units;
- return one or more units.

A lend operation fails when the desired quantity is less than or equal to zero or there are no enough available copies/seats.

A return operation fails when the desired quantity is less than or equal to zero or more units than those lent/active are returned.

The number of holdings can not be less than the number of lent or active units.

It is also connected with the Q8 cancelling. The purchase can not be canceled when there are still copies/electronic seats available from it. The availabilitycheck runs before the budget refund, so the partial change in the budget does not happen during a failed cancelation.

Digital media like EBooks and AudioBooks always have the concurrent-session model because of the method `isDigital()`, whereas print media have copy-lending.

## Q15 - Currency-Aware Money

The Money object now contains both:
- fixed-point value of the money in minor units; and
- three-letter currency identifier.

INR is set as the default currency to maintain backward compatibility with old code.

Currency identifiers are case-normalized to upper case and validated for having exactly three letters.

Money arithmetic requires currencies to match. Adding/subtracting different currencies raises std::invalid_argument.

Similarly, relational operators and ==/!= operators fail on mismatched currencies so monetary values of different currencies are not accidentally compared.

Scalar multiplication of a Money object keeps the currency as well.

The currency is kept throughout:
- book and journal bulk purchases;
- hardcover surcharge calculation;
- electronic half-price seat calculation;
- tax calculation;
- budget spending and usage;
- refund;
- end-of-year rollover;
- vendor prices;
- acquisition total.

Budget quota currencies must match those of the respective budgets, and budgets will not allow purchases/refunds using another currency.

Testing of full acquisition workflow using USD currency has been done, including vendor selection, tax, quota check, budget commitment, acquisition total,
and budget balance.

Mixed-currency acquisitions are refused before making any changes in holdings or budget state.

## Q16 - Replace Custom Test Macros with assert

The custom macros CHECK and CHECK_THROWS were replaced with their counterparts from the C++ standard library.

Regular tests now employ the standard C++ assert macro from <cassert>.

Tests for exceptions employ the small assertThrows function template, which runs the supplied callable and checks that the correct exception type was raised by standard assert.

Old global variables for test count and failures were deleted because standard assert exits immediately on a failed assertion.

The test runner now calls all test functions and outputs:

All tests passed.

only when all assertions pass.

The meanings of old tests were preserved, including tests for particular exception types.


## Commands Used

The following commands can be used in sequence to build, test, and demonstrate the project.

1. Remove the old build directory to ensure a clean build:

   `rm -rf build`

2. Configure the project using CMake:

   `cmake -S . -B build`

3. Compile the project:

   `cmake --build build`

4. Run the complete test suite:

   `./build/bookmgmt_tests`

5. Run the demonstration program:

   `./build/demo`


## ChatGPT / LLM Usage

This is my assignment. Read the assignment carefully and first help me understand what the project is about, what files are already given, what each class is supposed to do, and what the important rules and restrictions are. Do not directly change everything at once. First explain each question one by one and then help me implement them in order.

## ChatGPT / LLM Usage

- [Passed the assignment PDF and starter project]
  Help me understand the overall assignment, project structure, important rules, and what each major question is asking before I start implementing.

- [Passed the details for Journal / EBook / AudioBook / Thesis]
  Explain how these new resource types should fit into the existing class hierarchy and which existing classes can be reused through inheritance.

- [Passed the pricing and tax related questions]
  Help me understand where pricing logic should be implemented so that hardcover surcharge, bulk discount, electronic seat pricing, vendor pricing, and taxes work correctly without breaking polymorphism.

- [Passed the budget, quota, cancellation, and department related questions]
  Explain how the Budget and AcquisitionManager classes should interact, especially for quota checking, refunds, title limits, department-wise budgets, and cancellation of previous purchases.

- [Passed the batch-processing question]
  Help me understand how to implement all-or-nothing batch processing so that all requests are validated first and no real state is changed if one request fails.

- [Passed the currency-related question and an error message]
  I was getting `currency mismatch: INR vs USD` during a USD purchase. Help me identify where the default INR value was being created and explain how to preserve the correct currency through budget, tax, vendor, and acquisition calculations.

- [Passed the test file for the final question]
  Help me replace the custom test macros with standard C++ `assert` while keeping the same test behaviour, including exception checks.

- Help me understand and use the build and testing commands such as:
  `cmake -S . -B build`,
  `cmake --build build`,
  `./build/bookmgmt_tests`,
  `./build/demo`,
  and explain what each command is doing.

