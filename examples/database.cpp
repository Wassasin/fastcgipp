#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <fastcgi++/mysql.hpp>
#include <fastcgi++/manager.hpp>
#include <fastcgi++/request.hpp>


void error_log(const char* msg)
{
	using namespace std;
	using namespace boost;
	static ofstream error;
	if(!error.is_open())
	{
		error.open("/tmp/errlog", ios_base::out | ios_base::app);
		error.imbue(locale(error.getloc(), new posix_time::time_facet()));
	}

	error << '[' << posix_time::second_clock::local_time() << "] " << msg << endl;
}

struct Log: public Fastcgipp::Sql::Data::Set
{
public:
	size_t numberOfSqlElements() const { return 4; }

	Fastcgipp::Sql::Data::Index getSqlIndex(const size_t index) const
	{
		switch(index)
		{
			case 0:
				return ipAddress.getInt();
			case 1:
				return timestamp;
			case 2:
				return sessionId;
			case 3:
				return referral;
			default:
				return Fastcgipp::Sql::Data::Index();
		}
	}

	Fastcgipp::Http::Address ipAddress;
	Fastcgipp::Sql::Data::Datetime timestamp;
	Fastcgipp::Http::SessionId sessionId;
	Fastcgipp::Sql::Data::WtextN referral;
};


class Database: public Fastcgipp::Request<wchar_t>
{
	static const char insertStatementString[];
	static const char selectStatementString[];

	static Fastcgipp::Sql::MySQL::Connection sqlConnection;
	static Fastcgipp::Sql::MySQL::Statement insertStatement;
	static Fastcgipp::Sql::MySQL::Statement selectStatement;

	enum Status { START, FETCH } status;

	boost::shared_ptr<Fastcgipp::Sql::Data::SetContainer<Log> > selectSet;
	boost::shared_ptr<long long unsigned> rows;

	bool response();
public:
	Database(): status(START), selectSet(new Fastcgipp::Sql::Data::SetContainer<Log>), rows(new long long unsigned(0)) {}
	static void initSql();
};

const char Database::insertStatementString[] = "INSERT INTO logs (ipAddress, timeStamp, sessionId, referral) VALUE(?, ?, ?, ?)";
const char Database::selectStatementString[] = "SELECT SQL_CALC_FOUND_ROWS ipAddress, timeStamp, sessionId, referral FROM logs ORDER BY timeStamp DESC LIMIT 10";

Fastcgipp::Sql::MySQL::Connection Database::sqlConnection(1, 1);
Fastcgipp::Sql::MySQL::Statement Database::insertStatement(Database::sqlConnection);
Fastcgipp::Sql::MySQL::Statement Database::selectStatement(Database::sqlConnection);

void Database::initSql()
{
	sqlConnection.connect("localhost", "fcgi", "databaseExample", "fastcgipp", 0, 0, 0, "utf8");
	insertStatement.init(insertStatementString, sizeof(insertStatementString), &Log(), 0);
	selectStatement.init(selectStatementString, sizeof(selectStatementString), 0, &Log());
	sqlConnection.start();
}

bool Database::response()
{
	switch(status)
	{
		case START:
		{
			selectStatement.queue(EMPTY_SQL_SET, selectSet, EMPTY_SQL_INT, rows, callback);
			status=FETCH;
			return false;
		}
		case FETCH:
		{
			out << "Content-Type: text/html; charset=utf-8\r\n\r\n\
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\
	<head>\n\
		<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />\n\
		<title>fastcgi++: SQL Database example</title>\n\
	</head>\n\
	<body>\n\
		<h2>Showing " << selectSet->size() << " results out of " << *rows << "</h2>\n\
		<table>\n\
			<tr>\n\
				<td><b>IP Address</b></td>\n\
				<td><b>Timestamp</b></td>\n\
				<td><b>Session ID</b></td>\n\
				<td><b>Referral</b></td>\n\
			</tr>\n";

			for(Fastcgipp::Sql::Data::SetContainer<Log>::iterator it(selectSet->begin()); it!=selectSet->end(); ++it)
			{
				out << "\
			<tr>\n\
				<td>" << it->ipAddress << "</td>\n\
				<td>" << it->timestamp << "</td>\n\
				<td>" << it->sessionId << "</td>\n\
				<td>" << it->referral << "</td>\n\
			</tr>\n";
			}

			out << "\
		</table>\n\
		<p><a href='database.fcgi'>Refer Me</a></p>\n\
	</body>\n\
</html>";
			
			boost::shared_ptr<Log> queryParameters(new Log);
			queryParameters->ipAddress = environment.remoteAddress;
			queryParameters->timestamp = boost::posix_time::second_clock::universal_time();
			if(environment.referer.size())
				queryParameters->referral = environment.referer;
			else
				queryParameters->referral.nullness = true;

			insertStatement.queue(queryParameters, EMPTY_SQL_CONT, EMPTY_SQL_INT, EMPTY_SQL_INT, callback);

			return true;
		}
	}
}

int main()
{
	try
	{
		Database::initSql();
		Fastcgipp::Manager<Database> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
