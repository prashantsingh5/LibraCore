from datetime import datetime, timezone
from typing import Annotated, Literal
from pydantic import BaseModel, ConfigDict, Field, field_validator

Id = Annotated[str, Field(min_length=1, max_length=200, pattern=r'.*\S.*')]
Positive = Annotated[int, Field(strict=True, gt=0, le=2147483647)]
Minor = Annotated[int, Field(strict=True, ge=0, le=9223372036854775807)]

class Input(BaseModel):
    model_config = ConfigDict(extra='forbid')

class Page(Input):
    limit: int = Field(50, ge=1, le=200)
    offset: int = Field(0, ge=0)

class Search(Page):
    title: str = ''
    author: str = ''
    identifier: str = ''
    category: int = Field(-1, ge=-1, le=3)
    from_year: int = -2147483648
    to_year: int = 2147483647
    sort: Literal['id','title_asc','title_desc','year_asc','year_desc','price_asc','price_desc','rank'] = 'id'
    currency: str = ''
    q: str = ''

class ResourceId(Input):
    resource_id: Id

class UserId(Page):
    user_id: Id

class OrderId(Input):
    order_id: Positive

class LoanId(Input):
    loan_id: Id

class Department(Input):
    department_id: str = ''

class Purchase(ResourceId):
    quantity: Positive
    department_id: str = ''
    vendor_selection: Literal['Cheapest','Fastest','HighestRated'] = 'Cheapest'

class Submit(Purchase):
    request_id: Id

class Cancel(OrderId):
    request_id: Id

class Borrow(ResourceId):
    borrower_id: Id
    quantity: Positive = 1
    borrowed_at: datetime | None = None
    due_at: datetime | None = None
    request_id: Id

    @field_validator('borrowed_at', 'due_at')
    @classmethod
    def utc(cls, value):
        if value is not None:
            if value.tzinfo is None:
                raise ValueError('Timezone-aware timestamp required')
            return value.astimezone(timezone.utc)
        return value

class Return(LoanId):
    quantity: Positive
    returned_at: datetime | None = None
    request_id: Id
    _utc = field_validator('returned_at')(Borrow.utc.__func__)

class Resource(Input):
    resource_id: Id
    resource_type: Literal['Book','Journal','ElectronicResource','EBook','AudioBook','Thesis']
    title: Annotated[str, Field(min_length=1)]
    publisher: str = ''
    publication_year: int = 2026
    currency: str = Field('INR', pattern=r'^[A-Za-z]{3}$')
    unit_price_minor: Minor
    authors: list[str] = Field(default_factory=list)
    isbn: str = ''
    issn: str = ''
    edition: Positive = 1
    binding: int = Field(0, ge=0, le=1)
    issues_per_year: int = 1
    subscription_years: Positive = 1
    access_url: str = ''
    license: int = Field(1, ge=0, le=1)
    platform_fee_minor: Minor = 0
    platform_currency: str | None = None
    format: int = Field(0, ge=0, le=2)
    drm: bool = False
    narrator: str = ''
    duration_minutes: Positive = 1
    university: str = ''
    degree: str = ''
    supervisor: str = ''

class User(Input):
    user_id: Id
    name: Id
    email: Annotated[str, Field(min_length=3)]
    role: Literal['Student','Faculty','Librarian','DepartmentManager','Administrator'] = 'Student'
    department_id: str = ''

class DepartmentCreate(Input):
    department_id: Id
    name: Id

class BudgetCreate(Department):
    budget_id: Id
    fiscal_year: int
    currency: str = 'INR'
    total_minor: Minor

class Quota(Department):
    category: int = Field(ge=0, le=3)
    max_units: int = Field(ge=0, le=2147483647)
    max_spend: Minor
    max_titles: int | None = Field(None, ge=0)

class Rollover(Department):
    fiscal_year: int
    new_allocation: Minor
    carry_percent: int = Field(ge=0, le=100)
    request_id: Id

class Batch(Input):
    requests: list[Purchase] = Field(min_length=1, max_length=100)
    atomic: bool = True
    request_id: Id

class Offer(ResourceId):
    vendor_id: Id
    name: Id
    price_minor: Minor
    currency: str
    rating: float = Field(ge=0, le=5, allow_inf_nan=False)
    estimated_delivery_days: int = Field(ge=0)

class AssistantMessage(Input):
    message: str = Field(min_length=1, max_length=8000)
    session_id: Id

class Confirm(Input):
    token: Id
    session_id: Id

class ToolOutput(BaseModel):
    operation: str
    result: dict | list

# Outputs are validated independently from the model's prose. Optional fields retain
# forward compatibility while required financial/state fields are explicit.
class Output(BaseModel):
    model_config = ConfigDict(extra='allow')

class MoneyOutput(Output):
    minor_units: int
    currency: str
    formatted: str

class ResourceOutput(Output):
    resource_id: str
    resource_type: str
    title: str
    price: MoneyOutput

class SearchOutput(Output):
    items: list[ResourceOutput]
    limit: int
    offset: int

class AvailabilityOutput(Output):
    resource_id: str
    holdings: int
    borrowed: int
    available: int

class BudgetOutput(Output):
    total: MoneyOutput
    spent: MoneyOutput
    remaining: MoneyOutput
    percentage_used: float
    categories: list[dict]

class QuoteOutput(Output):
    cost: MoneyOutput

class CheckOutput(QuoteOutput):
    allowed: bool
    reason: str

class OrderOutput(Output):
    order_id: int
    quantity: int
    status: int
    approved: bool
    cost: MoneyOutput

class LoanOutput(Output):
    loan_id: str
    resource_id: str
    borrower_id: str
    quantity: int
    returned_quantity: int
    borrowed_at: datetime
    due_at: datetime
    returned_at: datetime | None
    status: int

class EventOutput(Output):
    event_id: int
    event_type: int
    resource_id: str
    quantity: int
    actor: str
    reference_id: str
    occurred_at: datetime
